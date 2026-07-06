#include "ServiceRegistry.hpp"
#include "logger/Log.hpp"

#include <algorithm>
#include <yaml-cpp/yaml.h>

using namespace lifecyclemgr;

namespace adsw_manager::agl {

    bool ServiceRegistry::LoadFromYaml(const std::string& path) {
        try {
            YAML::Node config = YAML::LoadFile(path);
            if (!config["services"]) {
                fprintf(stderr, "[ServiceRegistry] No 'services' key in YAML: %s\n", path.c_str());
                return false;
            }

            for (const auto& entry : config["services"]) {
                std::string name = entry["name"].as<std::string>();

                std::string st_str = entry["service_type"] ? entry["service_type"].as<std::string>() : "NORMAL";
                std::string rp_str = entry["recovery_policy"] ? entry["recovery_policy"].as<std::string>() : "R_DEFAULT";
                bool is_enabled    = entry["is_enabled"]      ? entry["is_enabled"].as<bool>()          : true;
                std::string os_str = entry["mw_type"]         ? entry["mw_type"].as<std::string>()       : "AGL";
                std::string is_str = entry["init_state"]      ? entry["init_state"].as<std::string>()    : "NOT_RUNNING";

                ServiceType service_type = ServiceType::NORMAL;
                if (st_str == "NORMAL")    service_type = ServiceType::NORMAL;

                RecoveryPolicy recovery_policy = RecoveryPolicy::R_DEFAULT;
                if      (rp_str == "R_DEFAULT")        recovery_policy = RecoveryPolicy::R_DEFAULT;
                else if (rp_str == "ALWAYS")           recovery_policy = RecoveryPolicy::ALWAYS;
                else if (rp_str == "ON_FAILURE")       recovery_policy = RecoveryPolicy::ON_FAILURE;
                else if (rp_str == "ON_SUCCESS")       recovery_policy = RecoveryPolicy::ON_SUCCESS;
                else if (rp_str == "ON_NOT_RESPONDING")recovery_policy = RecoveryPolicy::ON_NOT_RESPONDING;
                else if (rp_str == "MANUAL")           recovery_policy = RecoveryPolicy::MANUAL;

                MwType mw_type = MwType::AGL;
                if (os_str == "AGL")       mw_type = MwType::AGL;
                else if (os_str == "ROS2") mw_type = MwType::ROS2;

                ServiceState init_state = ServiceState::NOT_RUNNING;
                if (is_str == "RUNNING")   init_state = ServiceState::RUNNING;

                AppDockerInfo docker;
                if (entry["image"]) {
                    docker.image = entry["image"].as<std::string>();
                }
                if (entry["extra_args"]) {
                    for (const auto& arg : entry["extra_args"]) {
                        docker.extra_args.push_back(arg.as<std::string>());
                    }
                }
                if (entry["command_args"]) {
                    for (const auto& arg : entry["command_args"]) {
                        docker.command_args.push_back(arg.as<std::string>());
                    }
                }
                docker_info_.emplace(name, std::move(docker));

                Service svc;
                svc.name(name);
                svc.service_type(service_type);
                svc.recovery_policy(recovery_policy);
                svc.is_enabled(is_enabled);
                svc.mw_type(mw_type);
                // F009 (C-1): docker-backed apps carry container_name (== app name)
                // so lifecycle_mgr's DiagReport links the service to its container
                // card in lfc_monitor. Image-less apps leave it empty (service-only).
                if (entry["image"]) {
                    svc.container_name(name);
                }

                app_list_.try_emplace(std::move(svc), init_state);
            }

            fprintf(stdout, "[ServiceRegistry] Loaded %zu service(s) from %s\n",
                    app_list_.size(), path.c_str());
            return true;
        } catch (const YAML::Exception& e) {
            fprintf(stderr, "[ServiceRegistry] YAML parse error (%s): %s\n", path.c_str(), e.what());
            return false;
        }
    }

    bool ServiceRegistry::Initialize(const std::string& config_path) {
        app_list_.clear();
        docker_info_.clear();

        if (!config_path.empty()) {
            if (LoadFromYaml(config_path)) {
                return true;
            }
            fprintf(stderr, "[ServiceRegistry] Falling back to hardcoded list\n");
        }

        auto add = [this](std::string name,
                          ServiceType type,
                          RecoveryPolicy policy,
                          bool enabled,
                          MwType os,
                          ServiceState init_state) {
            Service svc;
            svc.name(std::move(name));
            svc.service_type(type);
            svc.recovery_policy(policy);
            svc.is_enabled(enabled);
            svc.mw_type(os);

            app_list_.try_emplace(std::move(svc), init_state);
        };

        add("adsw_monitor", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            MwType::AGL, ServiceState::NOT_RUNNING);

        add("ota_monitor", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            MwType::AGL, ServiceState::NOT_RUNNING);

        add("lfc_monitor", ServiceType::NORMAL,
            RecoveryPolicy::R_DEFAULT, true,
            MwType::AGL, ServiceState::NOT_RUNNING);

        add("hmi_launcher", ServiceType::NORMAL,
            RecoveryPolicy::ALWAYS, true,
            MwType::AGL, ServiceState::NOT_RUNNING);
        docker_info_["hmi_launcher"] = AppDockerInfo{
            "registry.obigo.com/oss/oss-hmi-launcher:latest",
            {
                "-it",
                "--net=host",
                "--privileged",
                "--restart", "unless-stopped",
                "--cap-add", "NET_RAW",
                "--cap-add", "NET_ADMIN",
                "--device", "/dev/dri:/dev/dri",
                "-v", "/run/user/1001:/run/user/1001",
                "-v", "/usr/share/flutter:/usr/share/flutter:ro",
                "-v", "/tmp/models:/tmp/models:ro",
                "-v", "/usr/share/fonts:/usr/share/fonts:ro",
                "-v", "/etc/fonts:/etc/fonts:ro",
                "-v", "/root/oss/hmi_launcher_config.json:/etc/hmi_launcher/config.json:rw",
                "-e", "HMI_LAUNCHER_CONFIG=/etc/hmi_launcher/config.json"
            },
            {"hmi_launcher"}
        };

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
            LOG_ERROR("[ServiceRegistry] " << app_name << " does not exist.");
            return std::nullopt;
        }

        return std::pair<Service, ServiceState>{ it->first, it->second };
    }

    std::vector<std::pair<std::string, ServiceState>> ServiceRegistry::GetNameStatePairs() const {
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
            LOG_ERROR("[ServiceRegistry] App not found: " << app_name);
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
            LOG_WARN("[ServiceRegistry] App already exists: " << info.name());
            return;
        }

        app_list_.emplace(info, init_state);
    }

    void ServiceRegistry::RemoveApp(const std::string& app_name) {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            LOG_ERROR("[ServiceRegistry] App not found: " << app_name);
            return;
        }

        app_list_.erase(it);
    }

    std::optional<AppDockerInfo> ServiceRegistry::GetDockerInfo(const std::string& app_name) const {
        auto it = docker_info_.find(app_name);
        if (it == docker_info_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void ServiceRegistry::UpdateAppState(const std::string& app_name, ServiceState state) {
        auto it = std::find_if(app_list_.begin(), app_list_.end(),
            [&](auto const& kv){ return kv.first.name() == app_name; });

        if (it == app_list_.end()) {
            LOG_ERROR("[ServiceRegistry] App not found in registry: " << app_name);
            return;
        }

        it->second = state;
    }

} // namespace adsw_manager::agl
