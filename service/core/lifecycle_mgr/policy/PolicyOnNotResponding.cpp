#include "PolicyOnNotResponding.hpp"

namespace lifecycle_manager {

PolicyOnNotResponding::PolicyOnNotResponding(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyOnNotResponding::~PolicyOnNotResponding() {}


ServiceControlAction PolicyOnNotResponding::Evaluate(std::shared_ptr<Service> service){
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    /* if service not responding sate .. then restart ?? */

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }
    switch(state){
        case ServiceState::NOT_RUNNING :{
        }break;

        case ServiceState::INITIALIZING :{
        }break;

        case ServiceState::RUNNING :{
        }break;

        case ServiceState::RUNNING_NO_RESPONSE :{
            // stop app and try run after terminated
            // start recovery
            action = ServiceControlAction::STOP; 
        }break;

        case ServiceState::TERMINATING :{
            action  = ServiceControlAction::START;
        }break;
    }

    return action;
}


} // namespace lifecycle_manager
