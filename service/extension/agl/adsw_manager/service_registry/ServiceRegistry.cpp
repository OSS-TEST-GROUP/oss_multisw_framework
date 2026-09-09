#include "ServiceRegistry.hpp"
#include "logger/Log.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <yaml-cpp/yaml.h>

using namespace lifecyclemgr;

namespace adsw_manager::agl {

// yaml 값의 `${VAR}` / `${VAR:-default}` 를 이 프로세스의 환경변수로 치환한다.
//
// 왜 필요한가: 서비스 목록의 **정본을 컨테이너 이미지로** 두려면, 보드마다 다른
// 값(호스트 docker gid, 런타임 uid)이 yaml 에 하드코딩돼 있으면 안 된다. 예전에는
// 그래서 보드가 호스트 파일을 이미지 사본 위에 마운트했고, 그 결과 목록의 정본이
// 이미지 밖으로 나가 **컨테이너를 업데이트해도 반영되지 않았다**(보드 81 실측
// 2026-09-07). 보드별 값만 환경변수로 빼면 목록 자체는 모든 보드에서 같아진다.
//
// ros2 쪽 adsw_manager 는 `ContainerController::RunCommand` 가 `system()` 이라
// docker 인자가 `/bin/sh` 를 통과해 확장이 공짜였다. 반면 AGL 쪽
// `AppController::RunDocker` 는 fork+execvp 로 셸을 거치지 않으므로 여기서
// 명시적으로 확장해야 한다.
//
// 정의되지 않은 변수는 `:-default` 가 있으면 그 값, 없으면 빈 문자열로 바뀐다
// (셸과 같은 규칙). 현재 yaml 은 모든 참조에 기존 하드코딩 값을 default 로 달아
// 두었으므로, 환경변수가 없는 배포에서도 동작이 종전과 같다.
std::string ExpandEnvVars(const std::string& in) {
    std::string out;
    out.reserve(in.size());

    for (size_t i = 0; i < in.size();) {
        if (in[i] != '$' || i + 1 >= in.size() || in[i + 1] != '{') {
            out.push_back(in[i++]);
            continue;
        }
        const size_t close = in.find('}', i + 2);
        if (close == std::string::npos) {
            // 닫는 중괄호가 없으면 치환하지 않고 원문을 남긴다.
            out.append(in, i, std::string::npos);
            break;
        }

        const std::string body = in.substr(i + 2, close - (i + 2));
        std::string name = body;
        std::string fallback;
        bool has_fallback = false;
        const size_t sep = body.find(":-");
        if (sep != std::string::npos) {
            name = body.substr(0, sep);
            fallback = body.substr(sep + 2);
            has_fallback = true;
        }

        const char* val = name.empty() ? nullptr : std::getenv(name.c_str());
        if (val != nullptr && *val != '\0') {
            out.append(val);
        } else if (has_fallback) {
            out.append(fallback);
        }
        i = close + 1;
    }
    return out;
}

namespace {

// yaml 시퀀스를 문자열 벡터로 읽으면서 환경변수를 확장한다.
void AppendExpanded(const YAML::Node& seq, std::vector<std::string>& out) {
    for (const auto& item : seq) {
        out.push_back(ExpandEnvVars(item.as<std::string>()));
    }
}

}  // namespace

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
                // image / extra_args / command_args 는 `${VAR:-default}` 를 지원한다
                // (ExpandEnvVars 주석 참조 — 목록의 정본을 이미지에 두기 위한 장치).
                if (entry["image"]) {
                    docker.image = ExpandEnvVars(entry["image"].as<std::string>());
                }
                if (entry["observe_only"]) {
                    docker.observe_only = entry["observe_only"].as<bool>();
                }
                if (entry["extra_args"]) {
                    AppendExpanded(entry["extra_args"], docker.extra_args);
                }
                if (entry["command_args"]) {
                    AppendExpanded(entry["command_args"], docker.command_args);
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
