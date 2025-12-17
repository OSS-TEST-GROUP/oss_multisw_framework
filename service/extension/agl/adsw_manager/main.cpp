#include "ADSWManager.hpp"
#include "service_monitor/ServiceMonitor.hpp"
#include "service_registry/ServiceRegistry.hpp"
#include "message_transmitter/MessageTransmitter.hpp"
#include "message_transmitter/DDSManager.hpp"
#include "service_controller/ServiceController.hpp"
#include "service_controller/AppController.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <memory>
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace adsw_manager::agl;

std::atomic<bool> running(true);

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Main] Termination signal received. Shutting down..." << std::endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    // 종료 신호 핸들러 등록
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    eprosima::fastdds::dds::DomainParticipantQos participant_qos;
    participant_qos.name("adsw_manager_participant");
    eprosima::fastdds::dds::DomainParticipant* participant = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    std::shared_ptr<ServiceRegistry> registry = std::make_shared<ServiceRegistry>();
    std::shared_ptr<AppController> app_controller = std::make_shared<AppController>();
    std::shared_ptr<ServiceController> service_controller = std::make_shared<ServiceController>(app_controller.get());
    std::shared_ptr<DDSManager> dds_manager = std::make_shared<DDSManager>();
    std::shared_ptr<MessageTransmitter> transmitter = std::make_shared<MessageTransmitter>(registry, service_controller, dds_manager);
    std::shared_ptr<ServiceMonitor> monitor = std::make_shared<ServiceMonitor>();

    ADSWManager adsw_manager(participant, registry, app_controller, service_controller,
                              dds_manager, transmitter, monitor);

    if (!adsw_manager.Initialize()) {
        std::cerr << "[Main] ADSWManager initialization failed." << std::endl;
        return 1;
    }

    if (!adsw_manager.StartService()) {
        std::cerr << "[Main] Failed to start ADSWManager service." << std::endl;
        return 1;
    }

    std::cout << "[Main] ADSWManager running. Press Ctrl+C to exit." << std::endl;

    // 프로그램 유지 (1초 간격으로 대기)
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 종료 처리
    adsw_manager.StopService();

    std::cout << "[Main] ADSWManager stopped. Exiting." << std::endl;

    return 0;
}
