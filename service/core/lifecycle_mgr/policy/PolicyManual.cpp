#include "PolicyManual.hpp"

namespace lifecycle_manager {

PolicyManual::PolicyManual(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name){
}
PolicyManual::~PolicyManual() {}

ServiceControlAction PolicyManual::Evaluate(std::shared_ptr<Service> service){
    ServiceState state = service->GetServiceState();
    ServiceControlAction action = ServiceControlAction::NONE;

    if (service->GetServiceMode() != ServiceMode::NORMAL){
        return action;
    }

    /* manual policy run via operator command or ?? */

    return action;
}


} // namespace lifecycle_manager
