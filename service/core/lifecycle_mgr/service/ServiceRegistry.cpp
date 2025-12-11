#include <algorithm>
#include <iostream>
#include <set>

#include "ServiceRegistry.hpp"
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

    std::string group = newGroup();

    AddGroup(group);

    for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
        std::shared_ptr<Service> service = GetService(svc->name());

        if (service != nullptr){
            // update service;
        }else{
            // create new service
            service = std::make_shared<Service>(svc->name(), 
                                                TypeConvert::ToServiceType(svc->service_type()), 
                                                TypeConvert::ToRecoveryPolicy(svc->recovery_policy()), 
                                                svc->is_enabled(),
                                                TypeConvert::ToOsType(svc->os_type()));
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


} // namespace lifecycle_manager
