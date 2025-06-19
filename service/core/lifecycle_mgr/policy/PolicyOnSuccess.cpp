#include "PolicyOnSuccess.hpp"

namespace lifecycle_manager {

PolicyOnSuccess::PolicyOnSuccess(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyOnSuccess::~PolicyOnSuccess() {}

ServiceControlAction PolicyOnSuccess::Evaluate(std::shared_ptr<Service> service){
    
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    switch(state){
        case ServiceState::NOT_RUNNING :
        case ServiceState::INITIALIZING :
        case ServiceState::RUNNING :
        case ServiceState::RUNNING_NO_RESPONSE :
        case ServiceState::TERMINATING :
            action = ServiceControlAction::NONE;
    }

    return action;

}


} // namespace lifecycle_manager
