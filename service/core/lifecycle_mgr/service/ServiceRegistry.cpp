#include "ServiceRegistry.hpp"

namespace lifecycle_manager {

ServiceRegistry::ServiceRegistry() {}
ServiceRegistry::~ServiceRegistry() {}


bool ServiceRegistry::UpdateServiceList(){

}

void ServiceRegistry::AddGroup(std::string name){
    
}

void ServiceRegistry::RemoveGroup(std::string name){
    
}

bool ServiceRegistry::IsGroup(std::string name){
    
}


bool ServiceRegistry::AddService(std::string service, std::string group){
    
}

void ServiceRegistry::RemoveService(std::string name){
    
}


std::shared_ptr<Service> ServiceRegistry::GetService(std::string name){
    
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServicesByGroup(std::string group){
    
}

std::string ServiceRegistry::GetGroupNameByService(std::string service){
    
}


uint64_t ServiceRegistry::GetServiceNum(){
    
}

uint64_t ServiceRegistry::GetServiceNumInGroup(std::string group){
    
}

uint64_t ServiceRegistry::GetGroupNum(){
    
}


} // namespace lifecycle_manager
