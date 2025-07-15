#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>


#include "ServiceRegistry.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/TypeConvert.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"

#include "logger/Log.hpp"

using namespace lifecycle_manager;

namespace ros2 {
namespace adsw_manager {

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}

bool ServiceRegistry::LoadServiceList() {
    // Stub: Assume success, or extend for file/config source later
    LOG_DEBUG("ServiceRegistry::LoadServiceList start ");

    if (LoadServiceList("autoware-single-pod.yaml") == true){
        LOG_DEBUG("ServiceRegistry::LoadServiceList end ");

        // Notify the service state after loading the service list
        return ProcessUpdateServiceListRequest();
    }

    return true;

}

bool ServiceRegistry::LoadServiceList(const std::string& path) {
    LOG_DEBUG("ServiceRegistry::LoadServiceList path: " << path);

    try {

        YAML::Node config = YAML::LoadFile(path);
        auto containers = config["spec"]["containers"];
        for (auto container : containers) {
            auto name = container["name"].as<std::string>();
            auto command = container["command"];
            auto args = container["args"];
            std::cout << "Container name: " << name << std::endl;
            std::cout << "Command: ";
            for (auto c : command) std::cout << c.as<std::string>() << " ";
            std::cout << std::endl;
            std::cout << "Args: ";

            if (args && args.IsSequence()) {
                for (auto argStringNode : args) {
                    std::string script = argStringNode.as<std::string>();
                    std::cout << "Full shell command:\n" << script << std::endl;

                    // Extract ros2 launch components
                    auto components = extractRos2LaunchCommands(script);
                    std::cout << "\nExtracted Components:\n";
                    for (const auto& cmd : components) {
                        std::cout << "- " << std::get<0>(cmd) << std::endl;

                        AddGroup(std::get<1>(cmd));  // Add package as group

                        std::shared_ptr<Service> service = std::make_shared<Service>(
                            std::get<1>(cmd), 
                            ServiceType::NORMAL,
                            RecoveryPolicy::ALWAYS,
                            true  // Assume enabled by default
                        );

                        AddService(service, std::get<1>(cmd));
                    }
                }
            }

            // Further splitting of args (which contains your launch commands) would require string parsing:
            // e.g., use std::regex or string split on "ros2 launch ..."
        }        

    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to load service list from " << path << ": " << e.what());
        return false;
    }

    LOG_DEBUG("ServiceRegistry::LoadServiceList end ");
    return true;
}

// Utility to extract ros2 launch statements
std::vector<std::tuple<std::string, std::string, std::string, std::string>> ServiceRegistry::extractRos2LaunchCommands(const std::string& script) {
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> result;

    // Pattern to match: ros2 launch <pkg> <launch_file> [args...]
    std::regex pattern(R"(ros2\s+launch\s+([^\s]+)\s+([^\s]+)([^&\n]*)?)");
    std::smatch match;

    std::string::const_iterator searchStart(script.cbegin());
    while (std::regex_search(searchStart, script.cend(), match, pattern)) {
        std::string full_cmd = match[0];
        std::string pkg = match[1];
        std::string launch_file = match[2];
        std::string extra_args = match[3];

        result.push_back(std::make_tuple(full_cmd, pkg, launch_file, extra_args));
        searchStart = match.suffix().first;
    }
    return result;
}

void ServiceRegistry::AddGroup(const std::string& name) {
    if (group_.count(name) == 0) {
      group_[name] = {};
    }
}

void ServiceRegistry::RemoveGroup(const std::string& name) {
    if (group_.count(name) > 0) {
      group_.erase(name);
    }
}

bool ServiceRegistry::IsGroup(const std::string& name) {
    return group_.count(name) > 0;
}

bool ServiceRegistry::AddService(std::shared_ptr<Service> service, const std::string& group) {
    if (!service || group.empty()) return false;

    const std::string& name = service->GetName();

    // Check for duplicate service
    if (services_.find(name) != services_.end()) return false;

    AddGroup(group);  // ensure group exists

    services_[name] = service;
    group_[group].insert(service);

    return true;
}

void ServiceRegistry::RemoveService(const std::string& name) {
    auto it = services_.find(name);
    if (it == services_.end()) return;

    std::shared_ptr<Service> service = it->second;

    // Remove from group
    for (auto& [group_name, service_set] : group_) {
        auto svc_it = service_set.find(service);
        if (svc_it != service_set.end()) {
            service_set.erase(svc_it);
            break;
        }
    }

    services_.erase(it);
}

std::shared_ptr<Service> ServiceRegistry::GetService(const std::string& name) {
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServicesByGroup(const std::string& group) {
    std::vector<std::shared_ptr<Service>> result;
    auto iter = group_.find(group);
    if (iter == group_.end()) {
        return result;
    }

    for (auto& iter : iter->second) {
        result.push_back(iter);
    }
    return result;
}

std::string ServiceRegistry::GetGroupNameByService(std::shared_ptr<Service> service) {
    for (auto& iter : group_) {
        if (iter.second.count(service) > 0) {
            return iter.first;
        }
    }

    return "";
}

uint64_t ServiceRegistry::GetServicesNum() {
    return services_.size();
}

uint64_t ServiceRegistry::GetServiceNumInGroup(const std::string& group) {
    auto iter = group_.find(group);
    if (iter == group_.end()) {
        return 0;
    }
    
    return iter->second.size();
}

uint64_t ServiceRegistry::GetGroupNum() {
    return group_.size();
}

bool ServiceRegistry::ProcessGetServiceStateRequest(lifecyclemgr::GetServiceStateRequest* message) {
    if (!message) {
        LOG_ERROR("GetServiceStateRequest is null");
        return false;
    }

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    lifecyclemgr::GetServiceStateResponse response;

    std::vector<std::string> service_list = message->service_list();
    bool is_all_service = message->is_all_service(); 

    const char* topic = ADSWManager::Get()->GetTopic(MessageType::GET_SERVICE_STATE_RESPONSE);    
    std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

    for (const auto& service : service_list) {
    
        std::shared_ptr<Service> service_ptr = GetService(service);
        if (service_ptr != nullptr) {
            
            ServiceState state = service_ptr->GetServiceState();

            response.service_list().push_back(service);
            response.service_state_list().push_back(TypeConvert::ToServiceState(state));
            LOG_DEBUG("Service: " << service << " State: " << static_cast<int>(state));
        } else {
            LOG_ERROR("Service not found: " << service);
        }
    }

    if (endpoint != nullptr) {
        endpoint->Write(&response);
    }
    LOG_DEBUG("Service state response sent for " << service_list.size() << " services.");
    return true;
}

bool ServiceRegistry::ProcessUpdateServiceListRequest() {
    LOG_DEBUG("Processing UpdateServiceListRequest");

    lifecyclemgr::UpdateServiceListRequest message;

    for (auto& service : services_){
        std::shared_ptr<Service> service_ptr = service.second;
        if (service_ptr != nullptr) {
            lifecyclemgr::Service service_info;
            service_info.name() = service_ptr->GetName();
            service_info.service_type() = TypeConvert::ToServiceType(service_ptr->GetServiceType());
            service_info.recovery_policy() = TypeConvert::ToRecoveryPolicy(service_ptr->GetRecoveryPolicy());
            service_info.is_enabled() = service_ptr->IsEnabled();
            message.service_list().push_back(service_info);
        }   
    }

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    const char* topic = ADSWManager::Get()->GetTopic(MessageType::UPDATE_SERVICE_LIST_REQUEST);
    std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

    if (endpoint != nullptr) {
        endpoint->Write(&message);
        LOG_DEBUG("UpdateServiceListRequest sent with " << message.service_list().size() << " services.");
        return true;
    } else {
        LOG_ERROR("Failed to get DDSMessageEndpoint for UpdateServiceListRequest");
        return false;
    }
}

} // namespace adsw_manager
} // namespace ros2
