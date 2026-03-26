#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>


#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <optional>

#include "MWServiceManager.hpp"
#include "logger/Log.hpp"

namespace fs = std::filesystem;

namespace lifecycle_manager {

std::optional<std::string> getEnv(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::optional<std::string>(val) : std::nullopt;
}

MWServiceManager::MWServiceManager() {}
MWServiceManager::~MWServiceManager() {}


bool MWServiceManager::Initialize(){
    LOG_DEBUG("MWServiceManager::Initialize");

    std::string config_path = getEnv("MMF_CONFIG_PATH").value_or("config");
    config_path.append("/config_middleware.yaml");
    return LoadMWConfig(config_path);
}

bool MWServiceManager::Start(){
    LOG_DEBUG("MWServiceManager::Start");

    RunMWServiceManagement();

    return true;

}

void MWServiceManager::Stop(){

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

    // Parse diagnostic policies
    if (root["config_middleware"] && root["config_middleware"].IsSequence()) {
        YAML::Node middlewareNode = root["config_middleware"];
        for (const auto& serviceNode : middlewareNode) {
            // Required fields: code, name, rules, active
            if (!serviceNode["exepath"] || !serviceNode["name"]) {
                // Skip if essential fields are missing
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

            // Parse args if present
            if (serviceNode["args"] && serviceNode["args"].IsSequence()) {
                YAML::Node argsNode = serviceNode["args"];
                for (const auto& argNode : argsNode) {
                    // Each rule should have status and message (condition is optional list)
                    if (!argNode["arg_name"] || !argNode["arg_default"]) {
                        LOG_DEBUG("LoadConfig  missing arg_name or arg_default ");
                        continue; // skip rules missing essential fields
                    }

                    std::string nameStr = argNode["arg_name"].as<std::string>();
                    std::string descStr = argNode["description"].as<std::string>();
                    std::string typeStr = argNode["arg_type"].as<std::string>();
                    std::string defStr = argNode["arg_default"].as<std::string>();

                    // Create a new DiagPolicyRule object and add it to the policy
                    arguments.emplace_back(nameStr, descStr, typeStr, defStr);
                    // Assuming DiagPolicy has a method to add a rule (or accessible vector)
                }
            }
            
            // Create a new DiagPolicy object
            std::shared_ptr<MWService> serivceObj  = std::make_shared<MWService>(code, name, exepath, active, std::move(arguments));

            // Add the completed policy to the list (only if active or regardless? likely include all)
            mw_services_[name] = serivceObj;

            LOG_DEBUG("LoadConfig " << name << "::" << mw_services_.size());
        }
    }

    return true;
}

void MWServiceManager::StartMWService(std::shared_ptr<MWService> mw_service){

    std::vector<std::string> args = mw_service->GetArgs();
    pid_t child_pid = run_process(mw_service->GetName(), mw_service->GetPath(), args);

    if (child_pid > 0) {
        LOG_INFO("Spawned child process PID :: " << child_pid);
        mw_service->SetPID(child_pid);
    }
}

void MWServiceManager::StopMWService(std::shared_ptr<MWService> mw_service){

    pid_t child_pid = mw_service->GetPID();

    if (child_pid > 0){
        // Kill the child process if needed
        if (kill_process(child_pid)) {
            LOG_INFO("Killed child process PID "" " << child_pid);
        }

        // Optionally wait for child to avoid zombie
        int status;
        waitpid(child_pid, &status, 0);

        mw_service->SetPID(0);
    }

}

void MWServiceManager::RunMWServiceManagement(){

    for (auto& mws : mw_services_){
        LOG_INFO("Run MWService :: " << mws.first);

        if (mws.first.compare("lifecycle_mgr") == 0) {
            // Skip lifecycle_mgr service
            LOG_INFO("Skip lifecycle_mgr service :: " << mws.first);
            continue;
        }
        else{
            LOG_DEBUG("Starting MWService :: " << mws.first << " , " << mws.first.compare("lifecycle_mgr"));
            StartMWService(mws.second);
        }
    }

}

pid_t MWServiceManager::run_process(const std::string exename, const std::string exepath, 
                        const std::vector<std::string>& args) {

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        // In child process
        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(exepath.c_str()));
        for (const auto& arg : args)
            c_args.push_back(const_cast<char*>(arg.c_str()));
        c_args.push_back(nullptr); // execvp expects nullptr terminated array

        execvp(c_args[0], c_args.data());
        perror("execvp failed");
        exit(1);
    }
    // In parent process
    return pid;
}

bool MWServiceManager::IsK3sMode() const {
    return k3s_mode_;
}

bool MWServiceManager::kill_process(pid_t pid) {
    if (pid <= 0) {
        LOG_ERROR("Invalid PID: " << pid);
        return false;
    }
    if (kill(pid, SIGKILL) == 0) {
        return true;
    }
    perror("kill failed");
    return false;
}

} // namespace lifecycle_manager
