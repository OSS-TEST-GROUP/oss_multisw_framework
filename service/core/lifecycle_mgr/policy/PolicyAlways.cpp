#include "PolicyAlways.hpp"

#include <utility>

namespace lifecycle_manager {

PolicyAlways::PolicyAlways(RecoveryPolicy policy, std::string name) : PolicyBase(policy, std::move(name)) {
}

ServiceControlAction PolicyAlways::Evaluate(std::shared_ptr<Service> service){
    
    ServiceState state = service->GetServiceState();
    
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    // 모든 ServiceState 를 빠짐없이 열거한다. 결과 action 이 같은 상태가 여럿이라
    // bugprone-branch-clone 이 잡지만 합치지 않는다 — 이유는 policy/.clang-tidy 참조.
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
