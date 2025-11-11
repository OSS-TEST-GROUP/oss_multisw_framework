#include "PolicyManual.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

PolicyManual::PolicyManual(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyManual::~PolicyManual() {}

ServiceControlAction PolicyManual::Evaluate(std::shared_ptr<Service> service){
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    LOG_DEBUG("PolicyManual state = " << static_cast<int32_t>(state));

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    /* manual policy run via operator command or ?? */
    switch(state){
        case ServiceState::NOT_RUNNING :{
            action  = ServiceControlAction::NONE;
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
            action = ServiceControlAction::NONE; 
        }break;

        case ServiceState::TERMINATING :{
            action  = ServiceControlAction::NONE;
        }break;
    }
    
    return action;
}


} // namespace lifecycle_manager
