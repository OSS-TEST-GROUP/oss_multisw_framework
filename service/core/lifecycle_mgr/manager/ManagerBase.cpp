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

const char* ManagerBase::GetTopic(MessageType mtype){
    
    EndPointFactory* factory = lifecycle_.GetEndpointFactory();

    if (factory != nullptr){
        return (factory->GetTopic(mtype));
    }
    return nullptr;
}

} // namespace lifecycle_manager
