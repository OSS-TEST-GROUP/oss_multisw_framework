#include "PolicyOnNotResponding.hpp"

#include <utility>

namespace lifecycle_manager {

PolicyOnNotResponding::PolicyOnNotResponding(RecoveryPolicy policy, std::string name) : PolicyBase(policy, std::move(name)) {
}


ServiceControlAction PolicyOnNotResponding::Evaluate(std::shared_ptr<Service> service){
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    // 모든 ServiceState 를 빠짐없이 열거한다. 결과 action 이 같은 상태가 여럿이라
    // bugprone-branch-clone 이 잡지만 합치지 않는다 — 이유는 policy/.clang-tidy 참조.
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
