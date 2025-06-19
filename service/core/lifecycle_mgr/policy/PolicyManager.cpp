#include <iostream>

#include "PolicyManager.hpp"
#include "PolicyAlways.hpp"
#include "PolicyManual.hpp"
#include "PolicyOnFailure.hpp"
#include "PolicyOnNotResponding.hpp"
#include "PolicyOnSuccess.hpp"

namespace lifecycle_manager {

constexpr const char* const policy_always = "lifecyclemgr";

PolicyManager::PolicyManager() {}
PolicyManager::~PolicyManager() {}

bool PolicyManager::Start() {
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    policies_[RecoveryPolicy::ALWAYS] = std::make_shared<PolicyAlways>(RecoveryPolicy::ALWAYS, "Policy-Always");
    policies_[RecoveryPolicy::ON_FAILURE] = std::make_shared<PolicyOnFailure>(RecoveryPolicy::ON_FAILURE, "Policy-OnFailure");
    policies_[RecoveryPolicy::ON_SUCCESS] = std::make_shared<PolicyOnSuccess>(RecoveryPolicy::ON_SUCCESS, "Policy-OnSuccess");
    policies_[RecoveryPolicy::ON_NOT_RESPONDING] = std::make_shared<PolicyOnNotResponding>(RecoveryPolicy::ON_NOT_RESPONDING, "Policy-NotRespond");
    policies_[RecoveryPolicy::MANUAL] = std::make_shared<PolicyManual>(RecoveryPolicy::MANUAL, "Policy-Manual");

    return true;
}

void PolicyManager::Stop() {
    
}

std::shared_ptr<PolicyBase> PolicyManager::GetPolicy(RecoveryPolicy policy){

    auto iter = policies_.find(policy);

    if (iter != policies_.end()){
        return iter->second;
    }

    return nullptr;
}

    
std::shared_ptr<PolicyBase> PolicyManager::GetPolicy(std::string name){

    for (auto& iter : policies_){

        if (iter.second->GetName() == name){
            // match name
            return iter.second;
        }
    }

    return nullptr;
}

ServiceControlAction PolicyManager::Evaluate(std::shared_ptr<Service> service){

    RecoveryPolicy recovery = service->GetRecoveryPolicy();

    std::shared_ptr<PolicyBase> policy = GetPolicy(recovery);

    if (policy != nullptr){
        return policy->Evaluate(service);
    }

    return ServiceControlAction::NONE;
}


} // namespace lifecycle_manager
