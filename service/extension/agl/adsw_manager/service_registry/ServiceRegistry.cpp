#include "ServiceRegistry.hpp"

#include <iostream>
#include <algorithm>

using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool ServiceRegistry::Initialize() {

        // app_list_.try_emplace(Service{"adsw_monitor", ServiceType::NORMAL,  RecoveryPolicy::R_DEFAULT, true,  OsType::AGL},
        //                       ServiceState::NOT_RUNNING);

        // app_list_.try_emplace(Service{"map", ServiceType::NORMAL,  RecoveryPolicy::R_DEFAULT, true,  OsType::AGL},
        //                       ServiceState::NOT_RUNNING);

        // app_list_.try_emplace(Service{"radio", ServiceType::NORMAL,  RecoveryPolicy::R_DEFAULT, true,  OsType::AGL},
        //                       ServiceState::NOT_RUNNING);

        // return true;

        app_list_.clear();

        auto add = [this](std::string name,
                          ServiceType type,
                          RecoveryPolicy policy,
                          bool enabled,
                          OsType os,
                          ServiceState init_state){ 
            Service svc;       // 기본 생성
            svc.name(std::move(name));       // 각 필드 setter로 설정
            svc.service_type(type);
            svc.recovery_policy(policy);
            svc.is_enabled(enabled);
            svc.os_type(os);

            app_list_.try_emplace(std::move(svc), init_state);
        };

        add("adsw_monitor", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            OsType::AGL, ServiceState::NOT_RUNNING);

        add("map", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            OsType::AGL, ServiceState::NOT_RUNNING);

        add("radio", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            OsType::AGL, ServiceState::NOT_RUNNING);

        return true;
    }

    std::vector<Service> ServiceRegistry::GetAppList() const {

        std::vector<Service> out;
        out.reserve(app_list_.size());
        
        for (const auto& [svc, state] : app_list_) {
            out.push_back(svc);
        }
        
        return out;

    }

    std::optional<std::pair<Service, ServiceState>>
    ServiceRegistry::GetAppInfo(const std::string& app_name) const {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            std::cerr << "[ServiceRegistry] " << app_name << " does not exist." << std::endl;
            return std::nullopt;
        }

        return std::pair<Service, ServiceState>{ it->first, it->second };
    }

    std::vector<std::pair<std::string, ServiceState>> ServiceRegistry::GetNameStatePairs() const {
         // std::lock_guard<std::mutex> lk(mtx_); 
        std::vector<std::pair<std::string, ServiceState>> out;
        out.reserve(app_list_.size());
        
        for (const auto& [svc, state] : app_list_) {
            out.emplace_back(svc.name(), state); 
        }
        
        return out;
    }

    ServiceState ServiceRegistry::GetAppState(const std::string& app_name) const {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            std::cerr << "[ServiceRegistry] App not found: " << app_name << std::endl;
            return ServiceState::NOT_RUNNING;
        }

        return it->second;
    }

    void ServiceRegistry::AddApp(const Service& info) {
        AddApp(info, ServiceState::NOT_RUNNING);
    }

    void ServiceRegistry::AddApp(const Service& info, ServiceState init_state) {
        auto it = std::find_if(app_list_.begin(), app_list_.end(), 
                        [&](auto const& kv){ return kv.first.name() == info.name(); });

        if (it != app_list_.end()) {
            std::cerr << "[ServiceRegistry] App already exists: " << info.name() << std::endl;
            return;
        }

        app_list_.emplace(info, init_state);
    }

    void ServiceRegistry::RemoveApp(const std::string& app_name) {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            std::cerr << "[ServiceRegistry] App not found: " << app_name << std::endl;
            return;
        }

        app_list_.erase(it);
    }

    void ServiceRegistry::UpdateAppState(const std::string& app_name, ServiceState state) {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            std::cerr << "[ServiceRegistry] App not found in registry >>>> " << app_name << std::endl;
            return;
        }

        it->second = state;
    }

}