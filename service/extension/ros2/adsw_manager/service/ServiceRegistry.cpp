#include "ServiceRegistry.hpp"

using namespace lifecycle_manager;

namespace ros2 {
namespace adsw_manager {

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}

bool ServiceRegistry::LoadServiceList() {
    // Stub: Assume success, or extend for file/config source later
    return true;
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

} // namespace diagnostic_manager
} // namespace ros2
