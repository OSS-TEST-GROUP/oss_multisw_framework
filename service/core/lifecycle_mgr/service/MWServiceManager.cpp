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

#include "MWServiceManager.hpp"

namespace fs = std::filesystem;

namespace lifecycle_manager {

MWServiceManager::MWServiceManager() {}
MWServiceManager::~MWServiceManager() {}


bool MWServiceManager::Initialize(){
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return false;
}

bool MWServiceManager::Start(){

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;

}

void MWServiceManager::Stop(){

}

bool MWServiceManager::LoadMWConfig(std::string path){
    YAML::Node root;
    try {
        // Load the YAML configuration file
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        // Failed to open or parse the YAML file
        return false;
    }

    // Parse diagnostic policies
    if (root["config_middleware"] && root["config_middleware"].IsSequence()) {
        YAML::Node middlewareNode = root["config_middleware"];
        for (const auto& serviceNode : middlewareNode) {
            // Required fields: code, name, rules, active
            if (!serviceNode["exepath"] || !serviceNode["name"]) {
                // Skip if essential fields are missing
                continue;
            }
            int code = serviceNode["code"].as<int>();
            std::string name = serviceNode["name"].as<std::string>();
            std::string exepath = serviceNode["exepath"].as<std::string>();
            bool active = false;
            if (serviceNode["active"]) {
                active = serviceNode["active"].as<bool>();
            }
            
            std::vector<ArgumentConfig> arguments;

            // Parse args if present
            if (serviceNode["args"] && serviceNode["args"].IsSequence()) {
                YAML::Node argsNode = serviceNode["args"];
                for (const auto& argNode : argsNode) {
                    // Each rule should have status and message (condition is optional list)
                    if (!argsNode["arg_name"] || !argsNode["arg_default"]) {
                        continue; // skip rules missing essential fields
                    }

                    std::string nameStr = argsNode["arg_name"].as<std::string>();
                    std::string descStr = argsNode["description"].as<std::string>();
                    std::string typeStr = argsNode["arg_type"].as<std::string>();
                    std::string defStr = argsNode["arg_default"].as<std::string>();

                    // Create a new DiagPolicyRule object and add it to the policy
                    arguments.emplace_back(nameStr, descStr, typeStr, defStr);
                    // Assuming DiagPolicy has a method to add a rule (or accessible vector)
                }
            }
            
            // Create a new DiagPolicy object
            std::shared_ptr<MWService> serivceObj  = std::make_shared<MWService>(code, name, exepath, active, std::move(arguments));

            // Add the completed policy to the list (only if active or regardless? likely include all)
            mw_services_[name] = serivceObj;
        }
    }

    return true;
}

void MWServiceManager::StartMWService(std::shared_ptr<MWService> mw_service){

    std::vector<std::string> args = mw_service->GetArgs();
    pid_t child_pid = run_process(args);

    if (child_pid > 0) {
        std::cout << "Spawned child process PID: " << child_pid << std::endl;
        mw_service->SetPID(child_pid);
    }
}

void MWServiceManager::StopMWService(std::shared_ptr<MWService> mw_service){

    pid_t child_pid = mw_service->GetPID();

    if (child_pid > 0){
        // Kill the child process if needed
        if (kill_process(child_pid)) {
            std::cout << "Killed child process PID: " << child_pid << std::endl;
        }

        // Optionally wait for child to avoid zombie
        int status;
        waitpid(child_pid, &status, 0);

        mw_service->SetPID(0);
    }

}

void MWServiceManager::RunMWServiceManagement(){

}

pid_t MWServiceManager::run_process(const std::vector<std::string>& args) {

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        // In child process
        std::vector<char*> c_args;
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

bool MWServiceManager::kill_process(pid_t pid) {
    if (kill(pid, SIGKILL) == 0) {
        return true;
    }
    perror("kill failed");
    return false;
}

} // namespace lifecycle_manager
