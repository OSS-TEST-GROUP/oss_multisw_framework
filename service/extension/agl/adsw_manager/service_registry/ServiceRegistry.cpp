#include "ServiceRegistry.hpp"

#include <iostream>
#include <algorithm>

using namespace adsw_manager_agl;

namespace adsw_manager::agl {

    bool ServiceRegistry::Initialize() {

        AppInfo info;
        info.app_name("adsw_monitor");
        info.app_path("/");
        info.app_state(AppState::RESUMED);
        app_list_.push_back(info);

        info.app_name("localization");
        info.app_path("/");
        info.app_state(AppState::RESUMED);
        app_list_.push_back(info);

        info.app_name("helloooo");
        info.app_path("/");
        info.app_state(AppState::RESUMED);
        app_list_.push_back(info);

        return true;
    }

    std::vector<AppInfo> ServiceRegistry::GetAppList() {

        return app_list_;

    }

    void ServiceRegistry::AddApp(AppInfo& info) {

        for (const auto& app : app_list_) {
            if (app.app_name() == info.app_name()) {
                std::cerr << "[ServiceRegistry] App is already exist: " << info.app_name() << std::endl;
                return;
            }
        }

        app_list_.push_back(info);

    }

    void ServiceRegistry::RemoveApp(const std::string& app_name) {

        app_list_.erase(
            std::remove_if(app_list_.begin(), app_list_.end(),
                            [&](const AppInfo& app){ return app.app_name() == app_name; }),
            app_list_.end()
        );

    }

    void ServiceRegistry::UpdateAppState(const std::string& app_name, AppState state) {

        bool found = false;
        
        for (auto& app : app_list_) {
            if (app.app_name() == app_name) {
                app.app_state() = state;
                found = true;
                break;
            }
        }

        if(!found) {
            std::cerr << "App not found in registry >>>> " << app_name << std::endl;
        }

    }

    const AppInfo* ServiceRegistry::GetAppInfo(std::string app_name) {

        for (const auto& app_info : app_list_) {
            if (app_info.app_name() == app_name) {
                return &app_info;
            }
        }

        std::cerr << "[ServiceRegistry] " << app_name << " is not exist." << std::endl;
        return nullptr;
    }

}