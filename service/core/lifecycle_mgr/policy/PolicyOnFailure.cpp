#include "PolicyOnFailure.hpp"

namespace lifecycle_manager {

PolicyOnFailure::PolicyOnFailure(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyOnFailure::~PolicyOnFailure() {}

ServiceControlAction PolicyOnFailure::Evaluate(std::shared_ptr<Service> service){

    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    switch(state){
        case ServiceState::NOT_RUNNING :{
            // Restart only on abnormal exit (non-zero exit code).
            // exit_code == -1 means the service has never been started; treat
            // as the initial launch so we start it unconditionally.
            // exit_code == 0 means the service exited cleanly; OnFailure should
            // NOT restart it in that case.
            int32_t ec = service->GetExitCode();
            action = (ec != 0) ? ServiceControlAction::START : ServiceControlAction::NONE;
        }break;

        case ServiceState::INITIALIZING :{
            action = ServiceControlAction::NONE;
            // wait initialize finish
        }break;

        case ServiceState::RUNNING :{
            action = ServiceControlAction::NONE;
        }break;

        case ServiceState::RUNNING_NO_RESPONSE :{
            // Unresponsive service is treated as abnormal; stop it so it
            // transitions to NOT_RUNNING where the exit code will be checked.
            action = ServiceControlAction::STOP;
        }break;

        case ServiceState::TERMINATING :{
            // Wait for termination to complete; NOT_RUNNING will check exit code.
            action = ServiceControlAction::NONE;
        }break;
    }

    return action;
}


} // namespace lifecycle_manager
