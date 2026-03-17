#include "PolicyOnSuccess.hpp"

namespace lifecycle_manager {

PolicyOnSuccess::PolicyOnSuccess(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyOnSuccess::~PolicyOnSuccess() {}

ServiceControlAction PolicyOnSuccess::Evaluate(std::shared_ptr<Service> service){
    
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    /* if service not responding sate .. then restart ?? */

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    switch(state){
        case ServiceState::NOT_RUNNING :{
            action  = ServiceControlAction::START;
        }break;

        case ServiceState::INITIALIZING :{
            action = ServiceControlAction::NONE; 
            // wait initialize finish
        }break;

        case ServiceState::RUNNING :{
            // do nothing
            action = ServiceControlAction::NONE; 
        }break;

        case ServiceState::RUNNING_NO_RESPONSE :{
            // stop app and try run after terminated
            action = ServiceControlAction::STOP; 
        }break;

        case ServiceState::TERMINATING :{
            // Wait for termination to complete; NOT_RUNNING will trigger restart
            action  = ServiceControlAction::NONE;
        }break;
    }

    return action;
}

} // namespace lifecycle_manager
