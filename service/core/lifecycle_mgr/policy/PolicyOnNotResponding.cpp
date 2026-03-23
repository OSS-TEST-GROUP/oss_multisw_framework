#include "PolicyOnNotResponding.hpp"

namespace lifecycle_manager {

PolicyOnNotResponding::PolicyOnNotResponding(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyOnNotResponding::~PolicyOnNotResponding() {}


ServiceControlAction PolicyOnNotResponding::Evaluate(std::shared_ptr<Service> service){
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    switch(state){
        case ServiceState::NOT_RUNNING :{
            // Restart only if this is the initial start (last == NOT_RUNNING)
            // or if the service arrived here via a watchdog-triggered STOP command
            // (last == TERMINATING, which follows RUNNING_NO_RESPONSE → STOP).
            // A direct crash without a prior STOP (last == RUNNING) should NOT
            // trigger a restart under this policy.
            // NOTE: natural graceful exit (RUNNING → TERMINATING → NOT_RUNNING)
            // is indistinguishable from a watchdog stop using last_state alone;
            // full disambiguation requires exit-code information from the IDL.
            ServiceState last = service->GetLastServiceState();
            action = (last == ServiceState::NOT_RUNNING || last == ServiceState::TERMINATING)
                         ? ServiceControlAction::START : ServiceControlAction::NONE;
        }break;

        case ServiceState::INITIALIZING :{
            action = ServiceControlAction::NONE;
            // wait initialize finish
        }break;

        case ServiceState::RUNNING :{
            action = ServiceControlAction::NONE;
        }break;

        case ServiceState::RUNNING_NO_RESPONSE :{
            // Stop the unresponsive service; it will transition through
            // TERMINATING to NOT_RUNNING where the restart will be triggered.
            action = ServiceControlAction::STOP;
        }break;

        case ServiceState::TERMINATING :{
            // Wait for termination to complete.
            action = ServiceControlAction::NONE;
        }break;
    }

    return action;
}


} // namespace lifecycle_manager
