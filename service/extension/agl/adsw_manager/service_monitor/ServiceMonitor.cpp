#include "ServiceMonitor.hpp"

#include "common/AppUtils.hpp"

using namespace eprosima::fastdds::dds;
using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool ServiceMonitor::Initialize(DomainParticipant* participant, std::shared_ptr<ServiceRegistry> registry, std::shared_ptr<MessageTransmitter> transmitter) {

        participant_ = participant;
        service_registry_ = registry;
        message_transmitter_ = transmitter;

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

        auto now  = std::chrono::steady_clock::now();
        const auto& app_list = service_registry_->GetAppList();

        for (auto& entry : readers_) {
            const std::string& app_name = entry.first;
            auto& reader = entry.second;

            auto last_time = reader->GetLastReceivedTime();

            auto it = std::find_if(app_list.begin(), app_list.end(),
                           [&](const ServiceStatus& app) { return app.name() == app_name; });

            if (it == app_list.end()) {
                continue;
            }

            const ServiceStatus& app_info = *it;
            ServiceState expected_state = (now - last_time > timeout_) ? ServiceState::NOT_RUNNING : ServiceState::RUNNING;

            if (app_info.service_state() != expected_state) {
                std::cout << "[ServiceMonitor] App state update : [[" << app_info.name() << "]] " << app_info.service_state() << " >>> " << expected_state << std::endl;
                service_registry_->UpdateAppState(app_name, expected_state);
                message_transmitter_->PublishAppState();
            }
        }

    }

}