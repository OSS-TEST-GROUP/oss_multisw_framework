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

        // F009 (C-1): initial container-state snapshot so docker-backed AGL apps
        // surface as container cards in lfc_monitor even before any transition.
        message_transmitter_->PublishContainerState();

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
            const bool has_received = (last_time != std::chrono::steady_clock::time_point::min());
            const bool heartbeat_ok = has_received && (now - last_time <= timeout_);

            ServiceState expected;
            if (heartbeat_ok) {
                expected = ServiceState::RUNNING;
            } else if (use_docker_fallback_ && app_controller_) {
                const auto docker = service_registry_->GetDockerInfo(app_name);
                if (!docker || docker->image.empty()) {
                    expected = ServiceState::NOT_RUNNING;
                } else if (app_controller_->IsAppRunning(app_name)) {
                    expected = ServiceState::RUNNING;
                } else if (app_controller_->ShouldAutoRestartApp(app_name)) {
                    std::cout << "[ServiceMonitor] Restarting Docker-backed app : [["
                              << app_name << "]]" << std::endl;
                    expected = app_controller_->LaunchApp(app_name)
                                   ? ServiceState::RUNNING
                                   : ServiceState::NOT_RUNNING;
                } else {
                    expected = ServiceState::NOT_RUNNING;
                }
            } else {
                expected = ServiceState::NOT_RUNNING;
            }

            auto info = service_registry_->GetAppInfo(app_name);
            if (!info) {
                continue;  // app was removed from registry between iterations
            }

            const ServiceState current = info->second;

            if (current != expected) {
                std::cout << "[ServiceMonitor] App state update : [["
                          << info->first.name() << "]] "
                          << current << " >>> "
                          << expected << std::endl;

                service_registry_->UpdateAppState(app_name, expected);
            }
        }

        // Publish the full app-state snapshot (NotifyServiceState) every poll,
        // not only when a state transitions. NotifyServiceState is VOLATILE, so
        // an event-only publish is missed entirely by any subscriber that
        // connects after the last transition — which is exactly why
        // lfc_monitor, opened well after hmi_launcher last went RUNNING, kept
        // showing NOT_RUNNING. A periodic full-snapshot publish lets a
        // late-joining lfc_monitor / dds_bridge converge on the current state
        // within one poll. Mirrors PublishContainerState below.
        message_transmitter_->PublishAppState();

        // F009 (C-1): publish container state every poll (not only on transition)
        // so a late-joining lifecycle_mgr / dds_bridge / lfc_monitor always
        // converges on the current container view — mirrors ros2 adsw_manager's
        // ContainerHealthMonitor periodic publish (volatile QoS has no history).
        message_transmitter_->PublishContainerState();
    }

}
