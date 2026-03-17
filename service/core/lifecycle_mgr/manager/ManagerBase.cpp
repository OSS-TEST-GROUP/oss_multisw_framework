#include "ManagerBase.hpp"
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {

ManagerBase::ManagerBase(LifecycleManager& lifecycle) 
    : lifecycle_(lifecycle){

}
ManagerBase::~ManagerBase() {}


std::shared_ptr<DDSTransmitter> ManagerBase::GetTransmitter(){
    return lifecycle_.GetTransmitter();
}
std::shared_ptr<ServiceRegistry> ManagerBase::GetRegistry(){
    return lifecycle_.GetRegistry();
}
std::shared_ptr<PolicyManager> ManagerBase::GetPolicy(){
    return lifecycle_.GetPolicy();
}
    
std::shared_ptr<StateManager> ManagerBase::GetStateManager(){
    return lifecycle_.GetStateMgr();
}
    
const char* ManagerBase::GetTopic(MessageType mtype){
    
    EndPointFactory* factory = lifecycle_.GetEndpointFactory();

    if (factory != nullptr){
        return (factory->GetTopic(mtype));
    }
    return nullptr;
}

std::shared_ptr<Service> ManagerBase::GetService(std::string name){
    return lifecycle_.GetService(name);
}

bool ManagerBase::IsK3sMode(){
    return lifecycle_.IsK3sMode();
}

} // namespace lifecycle_manager
