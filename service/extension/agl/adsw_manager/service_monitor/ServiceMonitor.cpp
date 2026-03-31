#include "ServiceMonitor.hpp"

#include "common/AppUtils.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool ServiceMonitor::Initialize(DomainParticipant* participant, std::shared_ptr<ServiceRegistry> registry, std::shared_ptr<MessageTransmitter> transmitter, AppController* app_controller) {

        participant_ = participant;
        service_registry_ = registry;
        message_transmitter_ = transmitter;
        app_controller_ = app_controller;

        return true;
    }

    void ServiceMonitor::StartMonitoring() {

        auto apps = service_registry_->GetAppList();

        for (const auto& app : apps) {
            readers_[app.name()] = std::make_unique<AppStateReader>(participant_, app.name());
        }

        running_ = true;
        monitoring_thread_ = std::thread([this]() {
            while (running_) {
                CheckAppState();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

    }

    void ServiceMonitor::StopMonitoring() {

        running_ = false;

        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }

    }

    void ServiceMonitor::CheckAppState() {
        const auto now = std::chrono::steady_clock::now();

        for (auto& [app_name, reader] : readers_) {
            const auto last_time = reader->GetLastReceivedTime();
            const bool heartbeat_ok = (now - last_time <= timeout_);

            ServiceState expected;
            if (heartbeat_ok) {
                expected = ServiceState::RUNNING;
            } else if (use_docker_fallback_ && app_controller_) {
                expected = app_controller_->IsAppRunning(app_name)
                               ? ServiceState::RUNNING
                               : ServiceState::NOT_RUNNING;
            } else {
                expected = ServiceState::NOT_RUNNING;
            }

            auto info = service_registry_->GetAppInfo(app_name);

            const ServiceState current = info->second;

            if (current != expected) {
                std::cout << "[ServiceMonitor] App state update : [["
                          << info->first.name() << "]] "
                          << static_cast<int>(current) << " >>> "
                          << static_cast<int>(expected) << std::endl;

                service_registry_->UpdateAppState(app_name, expected);
                message_transmitter_->PublishAppState();
            }
        }

    }

}