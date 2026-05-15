#include <iostream>

#include <yaml-cpp/yaml.h>
#include <exception>
#include <optional>

#include "MWServiceManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

namespace {
std::optional<std::string> getEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::optional<std::string>(val) : std::nullopt;
}
}  // namespace

MWServiceManager::MWServiceManager() {}
MWServiceManager::~MWServiceManager() {}

bool MWServiceManager::Initialize(){
    LOG_DEBUG("MWServiceManager::Initialize");

    std::string config_path = getEnv("MMF_CONFIG_PATH").value_or("config");
    config_path.append("/config_middleware.yaml");
    return LoadMWConfig(config_path);
}

// F011: middleware services are now started by core_entrypoint.sh, not by
// lifecycle_mgr.  Start() is kept for API symmetry but does nothing.
bool MWServiceManager::Start(){
    LOG_DEBUG("MWServiceManager::Start — no-op (middleware spawned by container entrypoint)");
    return true;
}

void MWServiceManager::Stop(){
    LOG_DEBUG("MWServiceManager::Stop — no-op");
}

bool MWServiceManager::LoadMWConfig(std::string path){

    LOG_DEBUG("LoadConfig path " << path);

    YAML::Node root;
    try {
        // Load the YAML configuration file
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        // Failed to open or parse the YAML file
        LOG_ERROR("LoadConfig path " << path << ", error = " << e.msg);
        return false;
    }

    // Parse lifecycle_mgr_config
    if (root["lifecycle_mgr_config"]) {
        YAML::Node lcConfig = root["lifecycle_mgr_config"];
        if (lcConfig["k3s_mode"]) {
            k3s_mode_ = lcConfig["k3s_mode"].as<bool>();
        }
        LOG_DEBUG("LoadConfig k3s_mode = " << k3s_mode_);
    }

    // Parse the middleware service list.  Nothing here actually launches the
    // services anymore (see class header); we just populate mw_services_ so
    // future inspection / diagnostics can still see what the YAML declares.
    if (root["config_middleware"] && root["config_middleware"].IsSequence()) {
        YAML::Node middlewareNode = root["config_middleware"];
        for (const auto& serviceNode : middlewareNode) {
            if (!serviceNode["exepath"] || !serviceNode["name"]) {
                LOG_DEBUG("LoadConfig  missing exepath or name ");
                continue;
            }
            int code = serviceNode["code"].as<int>();
            std::string name = serviceNode["name"].as<std::string>();
            std::string exepath = serviceNode["exepath"].as<std::string>();
            bool active = false;
            if (serviceNode["active"]) {
                active = serviceNode["active"].as<bool>();
            }
            LOG_DEBUG("LoadConfig " << name << "::" << exepath);

            std::vector<ArgumentConfig> arguments;

            if (serviceNode["args"] && serviceNode["args"].IsSequence()) {
                YAML::Node argsNode = serviceNode["args"];
                for (const auto& argNode : argsNode) {
                    if (!argNode["arg_name"] || !argNode["arg_default"]) {
                        LOG_DEBUG("LoadConfig  missing arg_name or arg_default ");
                        continue;
                    }

                    std::string nameStr = argNode["arg_name"].as<std::string>();
                    std::string descStr = argNode["description"].as<std::string>();
                    std::string typeStr = argNode["arg_type"].as<std::string>();
                    std::string defStr = argNode["arg_default"].as<std::string>();

                    arguments.emplace_back(nameStr, descStr, typeStr, defStr);
                }
            }

            std::shared_ptr<MWService> serivceObj = std::make_shared<MWService>(
                code, name, exepath, active, std::move(arguments));

            mw_services_[name] = serivceObj;

            LOG_DEBUG("LoadConfig " << name << "::" << mw_services_.size());
        }
    }

    return true;
}

bool MWServiceManager::IsK3sMode() const {
    return k3s_mode_;
}

} // namespace lifecycle_manager
