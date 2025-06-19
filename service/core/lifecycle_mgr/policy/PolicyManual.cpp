#include "PolicyManual.hpp"

namespace lifecycle_manager {

PolicyManual::PolicyManual(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyManual::~PolicyManual() {}

ServiceControlAction PolicyManual::Evaluate(std::shared_ptr<Service> service){
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
