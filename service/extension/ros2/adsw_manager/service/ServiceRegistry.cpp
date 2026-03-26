#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "ServiceRegistry.hpp"

#include "ADSWManager.hpp"
#include "Container.hpp"
#include "control/ContainerController.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "service/TypeConvert.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/MessageQueue.hpp"

#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

std::optional<std::string> getEnv(const std::string &key) {
  const char *val = std::getenv(key.c_str());
  return val ? std::optional<std::string>(val) : std::nullopt;
}

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}

// constexpr const char *SAVED_REGISTRY_BACKUP =
// "config/saved-service-list.yaml";

bool ServiceRegistry::LoadServiceList() {
  // Stub: Assume success, or extend for file/config source later
  LOG_DEBUG("ServiceRegistry::LoadServiceList start ");

  yaml_path_ = getEnv("SAVED_REGISTRY_BACKUP").value_or("config");
  yaml_path_.append("/saved-service-list.yaml");

  ready_service_list_ = LoadServiceSaved(yaml_path_);

  if (ready_service_list_ == false) {
    std::string list_path = getEnv("SAVED_REGISTRY_BACKUP").value_or("config");
    list_path.append("/ros2-sample-speclist.yaml");
    ready_service_list_ = LoadServiceList(list_path);
  }

  LOG_DEBUG("ServiceRegistry::LoadServiceList success =? "
            << ready_service_list_);

  // Notify the service state after loading the service list
  return ready_service_list_;
}

bool ServiceRegistry::LoadServiceList(const std::string &path) {
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
      for (auto c : command)
        std::cout << c.as<std::string>() << " ";
      std::cout << std::endl;
      std::cout << "Args: ";

      if (args && args.IsSequence()) {
        for (auto argStringNode : args) {
          std::string script = argStringNode.as<std::string>();
          std::cout << "Full shell command:\n" << script << std::endl;

          // Extract ros2 launch components
          auto components = extractRos2LaunchCommands(script);
          std::cout << "\nExtracted Components:\n";
          for (const auto &cmd : components) {
            std::cout << "- " << std::get<0>(cmd) << std::endl;

            AddGroup(std::get<1>(cmd)); // Add package as group

            std::shared_ptr<Service> service = std::make_shared<Service>(
                std::get<1>(cmd), ServiceType::NORMAL, RecoveryPolicy::ALWAYS,
                true // Assume enabled by default
            );

            service->SetLaunchFile(std::get<2>(cmd));

            AddService(service, std::get<1>(cmd));
          }
        }
      }

      // Further splitting of args (which contains your launch commands) would
      // require string parsing: e.g., use std::regex or string split on "ros2
      // launch ..."
    }

  } catch (const YAML::Exception &e) {
    LOG_ERROR("Failed to load service list from " << path << ": " << e.what());
    return false;
  }

  LOG_DEBUG("ServiceRegistry::LoadServiceList end ");
  return true;
}

// Utility to extract ros2 launch statements
std::vector<std::tuple<std::string, std::string, std::string, std::string>>
ServiceRegistry::extractRos2LaunchCommands(const std::string &script) {
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
      result;

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

std::vector<std::tuple<std::string, std::string, std::string, std::string>>
ServiceRegistry::extractRos2RunCommands(const std::string &script) {
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
      result;

  // Pattern to match: ros2 run <pkg> <exe> [args...]
  std::regex pattern(R"(ros2\s+run\s+([^\s]+)\s+([^\s]+)([^&\n]*)?)");
  std::smatch match;

  std::string::const_iterator searchStart(script.cbegin());
  while (std::regex_search(searchStart, script.cend(), match, pattern)) {
    std::string full_cmd = match[0];
    std::string pkg = match[1];
    std::string exename = match[2];
    std::string extra_args = match[3];

    result.push_back(std::make_tuple(full_cmd, pkg, exename, extra_args));
    searchStart = match.suffix().first;
  }
  return result;
}

void ServiceRegistry::AddGroup(const std::string &name) {
  if (group_.count(name) == 0) {
    group_[name] = {};
  }
}

void ServiceRegistry::RemoveGroup(const std::string &name) {
  if (group_.count(name) > 0) {
    group_.erase(name);
  }
}

bool ServiceRegistry::IsGroup(const std::string &name) {
  return group_.count(name) > 0;
}

bool ServiceRegistry::AddService(std::shared_ptr<Service> service,
                                 const std::string &group) {
  if (!service || group.empty())
    return false;

  const std::string &name = service->GetName();

  // Check for duplicate service
  if (services_.find(name) != services_.end())
    return false;

  AddGroup(group); // ensure group exists

  services_[name] = service;
  group_[group].insert(service);

  return true;
}

void ServiceRegistry::RemoveService(const std::string &name) {
  auto it = services_.find(name);
  if (it == services_.end())
    return;

  std::shared_ptr<Service> service = it->second;

  // Remove from group
  for (auto &[group_name, service_set] : group_) {
    auto svc_it = service_set.find(service);
    if (svc_it != service_set.end()) {
      service_set.erase(svc_it);
      break;
    }
  }

  services_.erase(it);
}

std::shared_ptr<Service> ServiceRegistry::GetService(const std::string &name) {
  auto it = services_.find(name);
  if (it != services_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Service>>
ServiceRegistry::GetServicesByGroup(const std::string &group) {
  std::vector<std::shared_ptr<Service>> result;
  auto iter = group_.find(group);
  if (iter == group_.end()) {
    return result;
  }

  for (auto &iter : iter->second) {
    result.push_back(iter);
  }
  return result;
}

std::string
ServiceRegistry::GetGroupNameByService(std::shared_ptr<Service> service) {
  for (auto &iter : group_) {
    if (iter.second.count(service) > 0) {
      return iter.first;
    }
  }

  return "";
}

uint64_t ServiceRegistry::GetServicesNum() { return services_.size(); }

uint64_t ServiceRegistry::GetServiceNumInGroup(const std::string &group) {
  auto iter = group_.find(group);
  if (iter == group_.end()) {
    return 0;
  }

  return iter->second.size();
}

uint64_t ServiceRegistry::GetGroupNum() { return group_.size(); }

bool ServiceRegistry::ProcessGetServiceStateRequest(
    lifecyclemgr::GetServiceStateRequest *message) {
  if (!message) {
    LOG_ERROR("GetServiceStateRequest is null");
    return false;
  }
  if (TypeConvert::ToOsType(message->os_type()) == OsType::ROS2) {
    std::shared_ptr<DDSTransmitter> transmitter =
        ADSWManager::Get()->GetTransmitter();

    std::vector<std::string> service_list = message->service_list();
    bool is_all_service = message->is_all_service();

    LOG_DEBUG("GetServiceStateRequest: " << service_list.size()
                                         << " all ?: " << is_all_service);

    get_service_response_.service_list().clear();
    get_service_response_.service_state_list().clear();

    for (const auto &service : service_list) {

      std::shared_ptr<Service> service_ptr = GetService(service);
      if (service_ptr != nullptr) {

        ServiceState state = service_ptr->GetServiceState();

        get_service_response_.service_list().push_back(service);
        get_service_response_.service_state_list().push_back(
            TypeConvert::ToServiceState(state));
        LOG_DEBUG("Service: " << service
                              << " State: " << static_cast<int>(state));
      } else {
        LOG_ERROR("Service not found: " << service);
      }
    }

    return transmitter->SendMessage(
        MessageType::GET_SERVICE_STATE_RESPONSE,
        reinterpret_cast<void *>(&get_service_response_));
  }
  return true;
}

bool ServiceRegistry::HasRecoveryChanged() const {
  for (const auto& kv : services_) {
    const std::string& name = kv.first;
    RecoveryPolicy current = kv.second->GetRecoveryPolicy();
    auto it = last_sent_recovery_.find(name);
    // 처음 전송이거나 값이 달라진 경우
    if (it == last_sent_recovery_.end() || it->second != current) {
      return true;
    }
  }
  return false;
}

void ServiceRegistry::UpdateLastSentRecovery() {
  for (const auto& kv : services_) {
    last_sent_recovery_[kv.first] = kv.second->GetRecoveryPolicy();
  }
}

bool ServiceRegistry::ProcessUpdateServiceListRequest() {
  LOG_DEBUG("Processing UpdateServiceListRequest");

  if (ready_service_list_ == false) {
    LOG_ERROR("Service list not ready  ");
    return false;
  }

  // recovery 값이 변경된 경우에만 전송 (최초 전송은 항상 실행)
  if (!HasRecoveryChanged()) {
    LOG_DEBUG("UpdateServiceListRequest skipped: no recovery policy changes.");
    return true;
  }

  lifecyclemgr::UpdateServiceListRequest message;

  for (auto &service : services_) {
    std::shared_ptr<Service> service_ptr = service.second;
    if (service_ptr != nullptr) {

      lifecyclemgr::Service service_info;
      service_info.name() = service_ptr->GetName();
      service_info.service_type() =
          TypeConvert::ToServiceType(service_ptr->GetServiceType());
      service_info.recovery_policy() =
          TypeConvert::ToRecoveryPolicy(service_ptr->GetRecoveryPolicy());
      service_info.is_enabled() = service_ptr->IsEnabled();
      service_info.os_type() = TypeConvert::ToOsType(OsType::ROS2);
      message.service_list().push_back(service_info);
    }
  }

  std::shared_ptr<DDSTransmitter> transmitter =
      ADSWManager::Get()->GetTransmitter();
  const char *topic =
      ADSWManager::Get()->GetTopic(MessageType::UPDATE_SERVICE_LIST_REQUEST);
  std::shared_ptr<DDSMessageEndpoint> endpoint =
      transmitter->GetMessageEndpoint(topic);
  if (endpoint != nullptr) {
    endpoint->Write(&message);
    LOG_DEBUG("UpdateServiceListRequest sent with "
              << message.service_list().size() << " services.");
    UpdateLastSentRecovery();
    return true;
  }

  LOG_ERROR("Failed to get DDSMessageEndpoint for UpdateServiceListRequest");
  return false;
}

bool ServiceRegistry::Save() {
  YAML::Node config;

  config["version"] = "0.1";

  YAML::Node
      ndcontainers; // Use plural 'containers' to match convention and your YAML

  for (auto &group : group_) {
    YAML::Node ndcontainer;

    // 'group' key is not typical under each container if you use container name
    // as key, instead store container groups as map keys, but if you want
    // inside each container, keep it.
    ndcontainer["group"] = group.first;

    YAML::Node ndservices;

    for (auto &service : group.second) {
      YAML::Node ndservice;

      ndservice["package"] = service->GetPackage();
      ndservice["name"] = service->GetName();
      ndservice["launchfile"] = service->GetLaunchFile();

      // Convert integers to strings if you need consistent quoting; here leave
      // as int or convert explicitly
      ndservice["type"] = static_cast<int32_t>(service->GetServiceType());
      ndservice["recovery"] =
          static_cast<int32_t>(service->GetRecoveryPolicy());

      // YAML supports boolean natively; if you want string "true"/"false"
      // change accordingly
      ndservice["enabled"] = service->IsEnabled();

      ndservice["max"] = service->GetMaxNodes();

      ndservices.push_back(ndservice);
    }

    ndcontainer["services"] = ndservices;

    // Push each container as an element of a sequence
    ndcontainers.push_back(ndcontainer);
  }

  // Note: your original had a typo "container" (singular).
  // Typically the correct key would be "containers".
  config["spec"]["containers"] = ndcontainers;

  // Write YAML to file
  std::ofstream fout(yaml_path_.c_str());
  if (!fout) {
    std::cerr << "Error opening file for writing: " << yaml_path_ << std::endl;
    return false;
  }
  fout << config;
  fout.close();

  return true;
}

bool ServiceRegistry::LoadServiceSaved(const std::string &path) {
  LOG_DEBUG("ServiceRegistry::LoadServiceList path: " << path);
  try {
    YAML::Node config = YAML::LoadFile(path);

    auto specsNode = config["specs"];
    if (!specsNode || !specsNode.IsSequence()) {
      LOG_ERROR("Invalid or missing 'specs' sequence in YAML");
      return false;
    }

    for (const auto &spec : specsNode) {
      auto containersNode = spec["containers"];
      if (!containersNode || !containersNode.IsSequence()) {
        LOG_ERROR("Invalid or missing 'containers' sequence in spec");
        continue;
      }

      // containersNode is a sequence: each item is a container (group)
      std::shared_ptr<ROSServiceController> controller =
          ADSWManager::Get()->GetServiceController();
      if (controller == nullptr) {
        LOG_ERROR("ServiceController is not initialized.");
        return false;
      }
      std::shared_ptr<ContainerController> container_controller_ =
          controller->GetContainer();

      for (const auto &containerData : containersNode) {

        // Extract container metadata
        std::string groupname = containerData["name"].as<std::string>("");
        std::string image = containerData["image"].as<std::string>("");
        std::string deployFilePath =
            containerData["deploy-filepath"].as<std::string>("");
        std::string namespace_ = containerData["namespace"].as<std::string>("");
        std::string containerTypeStr =
            containerData["container-type"].as<std::string>("docker");

        if (namespace_.empty()) {
          namespace_ = "default";
          LOG_DEBUG("Using default namespace: " << namespace_);
        } else {
          LOG_DEBUG("Using specified namespace: " << namespace_);
        }

        ContainerType containerType = ContainerType::DOCKER;
        if (containerTypeStr == "k3s") {
          containerType = ContainerType::K3S;
        } else if (containerTypeStr == "docker-compose") {
          containerType = ContainerType::DOCKER_COMPOSE;
        } else {
          containerType = ContainerType::DOCKER;
        }

        if (!groupname.empty() && !image.empty()) {
          LOG_DEBUG("Container required fields valid");
          std::shared_ptr<Container> container = std::make_shared<Container>(
              groupname, image, deployFilePath, namespace_, containerType);
          LOG_DEBUG("Container created: " << groupname
                    << " image: " << image
                    << " type: " << containerTypeStr
                    << " deploy-filepath: " << deployFilePath);

          container_controller_->AddContainer(container);
        }

        auto servicesNode = containerData["services"];

        std::cout << "Group name: " << groupname << std::endl;
        AddGroup(groupname);

        if (servicesNode && servicesNode.IsSequence()) {
          for (const auto &serviceNode : servicesNode) {
            if (!serviceNode["name"]) {
              LOG_DEBUG("Load missing service name");
              continue;
            }

            std::string pkgStr = serviceNode["package"].as<std::string>("");
            std::string nameStr = serviceNode["name"].as<std::string>("");
            std::string lfileStr =
                serviceNode["launchfile"].as<std::string>("");
            std::string typeStr = serviceNode["type"].as<std::string>("");
            std::string recoStr = serviceNode["recovery"].as<std::string>("");
            std::string enableStr = serviceNode["enabled"].as<std::string>("");
            std::string maxStr = serviceNode["max"].as<std::string>("");

            ServiceType serviceType = ServiceType::NORMAL;
            RecoveryPolicy recoveryPolicy = RecoveryPolicy::R_DEFAULT;
            bool enabled = true;
            if (!typeStr.empty()) {
              if (typeStr == "NORMAL") {
                serviceType = ServiceType::NORMAL;
              } else if (typeStr == "ONESHOT") {
                serviceType = ServiceType::ONESHOT;
              }
            }

            if (!recoStr.empty()) {
              if (recoStr == "ALWAYS") {
                recoveryPolicy = RecoveryPolicy::ALWAYS;
              } else if (recoStr == "ON_FAILURE") {
                recoveryPolicy = RecoveryPolicy::ON_FAILURE;
              } else if (recoStr == "ON_SUCCESS") {
                recoveryPolicy = RecoveryPolicy::ON_SUCCESS;
              } else if (recoStr == "ON_NOT_RESPONDING") {
                recoveryPolicy = RecoveryPolicy::ON_NOT_RESPONDING;
              } else if (recoStr == "MANUAL") {
                recoveryPolicy = RecoveryPolicy::MANUAL;
              }
            }
            if (enableStr == "false") {
              enabled = false; // Default to false if not specified
            } else if (enableStr == "true") {
              enabled = true;
            }

            std::shared_ptr<Service> service = std::make_shared<Service>(
                nameStr, serviceType, recoveryPolicy, enabled);

            service->SetPackage(pkgStr);
            service->SetLaunchFile(lfileStr);
            AddService(service, groupname);
          }
        }
      }
    }
  } catch (const YAML::Exception &e) {
    LOG_ERROR("Failed to load service list from " << path << ": " << e.what());
    return false;
  }
  LOG_DEBUG("ServiceRegistry::LoadServiceList end ");
  return true;
}

} // namespace adsw_manager
} // namespace ros2
