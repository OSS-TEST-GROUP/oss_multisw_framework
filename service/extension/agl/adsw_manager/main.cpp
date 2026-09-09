#include "ADSWManager.hpp"
#include "service_monitor/ServiceMonitor.hpp"
#include "service_registry/ServiceRegistry.hpp"
#include "message_transmitter/MessageTransmitter.hpp"
#include "message_transmitter/DDSManager.hpp"
#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

#include <memory>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace adsw_manager::agl;

std::atomic<bool> running(true);

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("[Main] Termination signal received. Shutting down...");
        running = false;
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Config path 우선순위: --config arg > ADSW_CONFIG env > default
    std::string config_path;

    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--config") {
            config_path = argv[i + 1];
            break;
        }
    }

    if (config_path.empty()) {
        const char* env = std::getenv("ADSW_CONFIG");
        if (env) config_path = env;
    }

    if (config_path.empty()) {
        config_path = "config/agl-service-list.yaml";
    }

    LOG_INFO("[Main] Using config: " << config_path);

    eprosima::fastdds::dds::DomainParticipantQos participant_qos;
    participant_qos.name("adsw_manager_participant");
    // Per-board DDS-plane isolation. Precedence: MMF_DDS_DOMAIN_ID (per-service
    // override) > DDS_DOMAIN_ID (canonical DDS plane) > 0. Raw Fast-DDS ignores
    // ROS_DOMAIN_ID -- that is the separate ROS2 plane. Must match lifecycle_mgr's
    // value on the board.
    int mmf_domain = 0;
    if (const char* e = std::getenv("MMF_DDS_DOMAIN_ID")) mmf_domain = std::atoi(e);
    else if (const char* g = std::getenv("DDS_DOMAIN_ID")) mmf_domain = std::atoi(g);
    eprosima::fastdds::dds::DomainParticipant* participant =
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
            ->create_participant(mmf_domain, participant_qos);

    std::shared_ptr<ServiceRegistry>    registry           = std::make_shared<ServiceRegistry>();
    std::shared_ptr<AppController>      app_controller     = std::make_shared<AppController>();
    std::shared_ptr<ServiceController>  service_controller = std::make_shared<ServiceController>(app_controller.get());
    std::shared_ptr<DDSManager>         dds_manager        = std::make_shared<DDSManager>();
    std::shared_ptr<MessageTransmitter> transmitter        = std::make_shared<MessageTransmitter>(registry, service_controller, dds_manager);
    std::shared_ptr<ServiceMonitor>     monitor            = std::make_shared<ServiceMonitor>();

    ADSWManager adsw_manager(participant, registry, app_controller, service_controller,
                              dds_manager, transmitter, monitor);

    if (!adsw_manager.Initialize(config_path)) {
        LOG_ERROR("[Main] ADSWManager initialization failed.");
        return 1;
    }

    if (!adsw_manager.StartService()) {
        LOG_ERROR("[Main] Failed to start ADSWManager service.");
        return 1;
    }

    LOG_INFO("[Main] ADSWManager running. Press Ctrl+C to exit.");

    while (running) {
        adsw_manager.Process();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    adsw_manager.StopService();

    LOG_INFO("[Main] ADSWManager stopped. Exiting.");

    return 0;
}
