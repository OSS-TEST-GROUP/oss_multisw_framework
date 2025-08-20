#include "ServiceRegistry.hpp"

#include <iostream>
#include <algorithm>

using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool ServiceRegistry::Initialize() {

        ServiceStatus info;
        info.name("adsw_monitor");
        info.service_type(ServiceType::NORMAL);
        info.service_state(ServiceState::NOT_RUNNING);
        info.is_enabled(true);
        info.recovered_count(0);
        app_list_.push_back(info);

        info.name("map");
        info.service_type(ServiceType::NORMAL);
        info.service_state(ServiceState::NOT_RUNNING);
        info.is_enabled(true);
        info.recovered_count(0);
        app_list_.push_back(info);

        info.name("radio");
        info.service_type(ServiceType::NORMAL);
        info.service_state(ServiceState::NOT_RUNNING);
        info.is_enabled(true);
        info.recovered_count(0);
        app_list_.push_back(info);

        return true;
    }

    std::vector<ServiceStatus> ServiceRegistry::GetAppList() {

        return app_list_;

    }

    void ServiceRegistry::AddApp(ServiceStatus& info) {

        for (const auto& app : app_list_) {
            if (app.name() == info.name()) {
                std::cerr << "[ServiceRegistry] App is already exist: " << info.name() << std::endl;
                return;
            }
        }

        app_list_.push_back(info);

    }

    void ServiceRegistry::RemoveApp(const std::string& app_name) {

        app_list_.erase(
            std::remove_if(app_list_.begin(), app_list_.end(),
                            [&](const ServiceStatus& app){ return app.name() == app_name; }),
            app_list_.end()
        );

    }

    void ServiceRegistry::UpdateAppState(const std::string& app_name, ServiceState state) {

        bool found = false;
        
        for (auto& app : app_list_) {
            if (app.name() == app_name) {
                app.service_state() = state;
                found = true;
                break;
            }
        }

        if(!found) {
            std::cerr << "App not found in registry >>>> " << app_name << std::endl;
        }

    }

    const ServiceStatus* ServiceRegistry::GetAppInfo(std::string app_name) {

        for (const auto& app_info : app_list_) {
            if (app_info.name() == app_name) {
                return &app_info;
            }
        }

        std::cerr << "[ServiceRegistry] " << app_name << " is not exist." << std::endl;
        return nullptr;
    }

}