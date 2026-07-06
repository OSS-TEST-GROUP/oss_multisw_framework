#include <algorithm>
#include <iostream>
#include <set>

#include "ServiceRegistry.hpp"
#include "logger/Log.hpp"
#include "include/TypeConvert.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "transmitter/DDSTransmitter.hpp"

namespace lifecycle_manager {

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}

bool ServiceRegistry::UpdateServiceList(UpdateServiceListRequest* update){

    std::vector<lifecyclemgr::Service> *slist = &update->service_list();
    std::set<std::shared_ptr<Service>> service_set;

    // F013: refresh container registry first so the Service↔container link
    // can be validated against known metadata when we walk the service list.
    {
        std::vector<lifecyclemgr::Container>& clist = update->container_list();
        for (auto& c : clist) {
            const std::string name = c.name();
            if (name.empty()) continue;
            auto existing = GetContainer(name);
            if (existing) {
                existing->SetImage(c.image());
                existing->SetDeployFile(c.deploy_file());
                existing->SetNamespace(c.container_namespace());
                existing->SetType(TypeConvert::ToContainerType(c.type()));
            } else {
                auto fresh = std::make_shared<Container>(
                    name,
                    c.image(),
                    c.deploy_file(),
                    c.container_namespace(),
                    TypeConvert::ToContainerType(c.type()));
                UpsertContainer(fresh);
            }
            LOG_DEBUG("ServiceRegistry: container registered " << name);
        }
    }

    // Reuse the existing group if any service in the list is already registered.
    // Allocating a new group on every call would cause unbounded group growth
    // and StateMonitor would send redundant GetServiceState requests for empty
    // groups (DESIGN-1 fix).
    std::string group;
    for (auto svc = slist->begin(); svc != slist->end(); ++svc) {
        std::shared_ptr<Service> existing = GetService(svc->name());
        if (existing) {
            group = GetGroupNameByService(existing);
            if (!group.empty()) break;
        }
    }
    if (group.empty()) {
        group = newGroup();
        AddGroup(group);
    }

    for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
        std::shared_ptr<Service> service = GetService(svc->name());

        if (service != nullptr){
            // Update mutable properties — recovery policy, type, and enabled flag may
            // change when adsw_manager restarts with a new saved-service-list.yaml.
            service->SetServiceType(TypeConvert::ToServiceType(svc->service_type()));
            service->SetRecoveryPolicy(TypeConvert::ToRecoveryPolicy(svc->recovery_policy()));
            service->SetEnabled(svc->is_enabled());
            // F013: container reassignment is allowed on re-registration.
            service->SetContainerName(svc->container_name());
            LOG_DEBUG("ServiceRegistry: updated service " << svc->name());
        }else{
            // create new service
            service = std::make_shared<Service>(svc->name(),
                                                TypeConvert::ToServiceType(svc->service_type()),
                                                TypeConvert::ToRecoveryPolicy(svc->recovery_policy()),
                                                svc->is_enabled(),
                                                TypeConvert::ToMwType(svc->mw_type()));
            // F013: stamp container linkage from the wire.
            service->SetContainerName(svc->container_name());
            // New Service Add to Registry
            AddService(service, group);
        }
        service_set.insert(service);
        service->Print();
    }

    std::shared_ptr<DDSTransmitter> transmitter = LifecycleFactory::GetInstance()->GetTransmitter();

    bool retval = transmitter->SendMessage(MessageType::UPDATE_SERVICE_LIST_RESPONSE, 
                                    reinterpret_cast<void*>(&udpate_response_));

    if (service_set.size() > 0){
        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetStateMgr();
        if (manager != nullptr){
            manager->ProcessServiceHandling(service_set);
        }
    }
    return retval;       
}

void ServiceRegistry::AddGroup(std::string name){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    if (hasGroup(name)){
        // already had group name
    }

    groups_[name] = std::make_shared<std::vector<std::string>>();
}

void ServiceRegistry::RemoveGroup(std::string name){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);
 
    auto it = groups_.find(name);

    if (it != groups_.end()){
        groups_.erase(it);
    }
}

bool ServiceRegistry::IsGroup(std::string name){
    return hasGroup(name);
}


bool ServiceRegistry::AddService(std::shared_ptr<Service> service, std::string group){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    if (hasService(service->GetName())){
        // sevice alreay exist
    }    

    services_[service->GetName()] = service;

    std::shared_ptr<std::vector<std::string>> group_ptr = getGroup(group);

    if (group_ptr != nullptr){
        vectorAdd(*group_ptr, service->GetName());
    }

    return true;

}

void ServiceRegistry::RemoveService(std::string name){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    auto it = services_.find(name);

    if (it != services_.end()){
        services_.erase(it);
    }

    // need to remove from group
}


std::shared_ptr<Service> ServiceRegistry::GetService(std::string name){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    auto it = services_.find(name);

    if (it != services_.end()){
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServicesByGroup(std::string group){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    std::vector<std::shared_ptr<Service>> group_list;

    if (group.empty()){
        // return all 
        for (auto& iter : services_) {
            group_list.push_back(iter.second);
        }        
    }
    else
    {
        std::shared_ptr<std::vector<std::string>> group_ptr = getGroup(group);

        if (group_ptr != nullptr){

            for (auto iter = group_ptr->begin() ; iter != group_ptr->end() ; iter++) {

                std::shared_ptr<Service> service = getService(*iter);

                if (service){
                    group_list.push_back(service);
                }
            }
        }
    }

    return group_list;

}

std::vector<std::string> ServiceRegistry::GetServicesNameByGroup(std::string group){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    std::vector<std::string> group_list;

    if (group.empty()){
        // return all 
        for (auto& iter : services_) {
            group_list.push_back(iter.second->GetName());
        }           
    }
    else
    {
        std::shared_ptr<std::vector<std::string>> group_ptr = getGroup(group);

        if (group_ptr != nullptr){
            for (auto iter = group_ptr->begin() ; iter != group_ptr->end() ; iter++) {

                std::shared_ptr<Service> service = getService(*iter);

                if (service){
                    group_list.push_back(service->GetName());
                }
            }
        }

    }
    return group_list;

}

std::string ServiceRegistry::GetGroupNameByService(std::shared_ptr<Service> service){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    for (const auto &iter : groups_)
    {
        if (vectorHas(*iter.second, service->GetName())){
            // return this group
            return iter.first;
        }
    }
    return "";
}
    
std::vector<std::string> ServiceRegistry::GetGroupList(){
    std::vector<std::string> group_list;

    for (const auto &iter : groups_)
    {
        group_list.push_back(iter.first);
    }
    return group_list;
}


uint64_t ServiceRegistry::GetServiceNum(){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    return services_.size();

}

uint64_t ServiceRegistry::GetServiceNumInGroup(std::string group){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    if (group.size() == 0){
        return services_.size();
    }

    std::shared_ptr<std::vector<std::string>> group_ptr = getGroup(group);

    if (group_ptr){
        return group_ptr->size();
    }

    return 0;

}

uint64_t ServiceRegistry::GetGroupNum(){
    std::lock_guard<std::mutex> lock_guard(lock_mutex_);

    return groups_.size();
}


bool ServiceRegistry::hasService(std::string name){

    auto it = services_.find(name);

    if (it != services_.end()) {
        return true;
    }

    return false;
}


std::shared_ptr<Service> ServiceRegistry::getService(std::string name){

    auto it = services_.find(name);

    if (it != services_.end()) {
        return it->second;
    }

    return nullptr;
    
}


bool ServiceRegistry::hasGroup(std::string name){

    auto it = groups_.find(name);

    if (it != groups_.end()) {
        return true;
    }

    return false;
    
}

std::shared_ptr<std::vector<std::string>> ServiceRegistry::getGroup(std::string name){

    auto it = groups_.find(name);

    if (it != groups_.end()) {
        return it->second;
    }

    return nullptr;
    
}

bool ServiceRegistry::vectorHas(std::vector<std::string> vec, std::string item){
    
    if(std::find(vec.begin(), vec.end(), item) != vec.end()) { // vector내에 item 이 존재하지 않음
        return true;
    }

    return false;
}

bool ServiceRegistry::vectorAdd(std::vector<std::string>& vec, std::string item){
    
    if(std::find(vec.begin(), vec.end(), item) == vec.end()) { // vector내에 item 이 존재하지 않음
        vec.push_back(item);
        return true;
    }

    return false;
}

bool ServiceRegistry::vectorRemove(std::vector<std::string>& vec, std::string item){

    auto iter = std::find(vec.begin(), vec.end(), item);

    if (iter != vec.end()){
        // found .. remove this item
        vec.erase(iter);
        return true;
    }

    return false;

}

std::string ServiceRegistry::newGroup(){
    static int32_t groupid = 10;

    std::string groupname = "group_" + std::to_string(groupid);
    groupid += 10;

    return groupname;
}

// ── F013: Container registry API ────────────────────────────────────────

std::shared_ptr<Container> ServiceRegistry::GetContainer(const std::string& name) {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    auto it = containers_.find(name);
    return (it != containers_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Container>> ServiceRegistry::GetAllContainers() {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    std::vector<std::shared_ptr<Container>> out;
    out.reserve(containers_.size());
    for (auto& kv : containers_) out.push_back(kv.second);
    return out;
}

void ServiceRegistry::UpsertContainer(const std::shared_ptr<Container>& c) {
    if (!c) return;
    std::lock_guard<std::mutex> lock(lock_mutex_);
    containers_[c->GetName()] = c;
}

void ServiceRegistry::UpdateContainerState(const std::string& name,
                                           ContainerState state,
                                           int32_t exit_code) {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    auto it = containers_.find(name);
    if (it == containers_.end()) {
        // First time we see this container — auto-create a minimal record so
        // a notification arriving before UpdateServiceList still produces a
        // visible entry in lfc_monitor.
        auto fresh = std::make_shared<Container>(
            name, "", "", "", ContainerType::CT_UNKNOWN);
        fresh->SetState(state);
        fresh->SetExitCode(exit_code);
        containers_[name] = fresh;
        return;
    }
    it->second->SetState(state);
    it->second->SetExitCode(exit_code);
}

std::vector<std::string> ServiceRegistry::GetServicesInContainer(
    const std::string& container_name) {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    std::vector<std::string> out;
    for (auto& kv : services_) {
        if (kv.second && kv.second->GetContainerName() == container_name) {
            out.push_back(kv.first);
        }
    }
    return out;
}

} // namespace lifecycle_manager
