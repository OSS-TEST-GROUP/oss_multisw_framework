
#ifndef MOCK_LIFECYCLE_POLICY_BASE_HPP_
#define MOCK_LIFECYCLE_POLICY_BASE_HPP_


#include "policy/PolicyBase.hpp"
#include <gmock/gmock.h>


namespace lifecycle_manager {

class MockPolicyBase : public PolicyBase {
public:
    MockPolicyBase(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name) {}
    virtual ~MockPolicyBase() {}

    // Mock method for GetPolicy
    MOCK_METHOD(RecoveryPolicy, GetPolicy, (), ());
    // Mock method for GetName
    MOCK_METHOD(std::string, GetName, (), ());
    
    // Mock method for Evaluate

    MOCK_METHOD(ServiceControlAction, Evaluate, (std::shared_ptr<Service> service), (override));
};

MockPolicyBase* Mock_PolicyBase = nullptr;

PolicyBase::PolicyBase(RecoveryPolicy policy, std::string name) {
}
PolicyBase::~PolicyBase() {
    Mock_PolicyBase = nullptr;
}

RecoveryPolicy PolicyBase::GetPolicy() {
    return Mock_PolicyBase->GetPolicy();
}
std::string PolicyBase::GetName() {
    return Mock_PolicyBase->GetName();
}

ServiceControlAction PolicyBase::Evaluate(std::shared_ptr<Service> service) {
    return Mock_PolicyBase->Evaluate(service);
}

}
 // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_POLICY_BASE_HPP_
