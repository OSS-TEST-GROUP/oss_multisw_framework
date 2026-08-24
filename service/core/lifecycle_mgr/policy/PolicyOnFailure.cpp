#include "PolicyOnFailure.hpp"

#include <utility>

namespace lifecycle_manager {

PolicyOnFailure::PolicyOnFailure(RecoveryPolicy policy, std::string name) : PolicyBase(policy, std::move(name)) {
}

ServiceControlAction PolicyOnFailure::Evaluate(std::shared_ptr<Service> service){

    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    // 모든 ServiceState 를 빠짐없이 열거한다. 결과 action 이 같은 상태가 여럿이라
    // bugprone-branch-clone 이 잡지만 합치지 않는다 — 이유는 policy/.clang-tidy 참조.
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
