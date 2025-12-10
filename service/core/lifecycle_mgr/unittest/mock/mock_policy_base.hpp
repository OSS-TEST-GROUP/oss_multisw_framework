#ifndef MOCK_LIFECYCLE_POLICY_BASE_HPP_
#define MOCK_LIFECYCLE_POLICY_BASE_HPP_

#include <gmock/gmock.h>
#include "policy/PolicyBase.hpp"

namespace lifecycle_manager {
class MockPolicyBase : public PolicyBase {
public:
    MockPolicyBase(RecoveryPolicy policy, std::string name) : PolicyBase(policy, name) {}
    ~MockPolicyBase() = default;

    // Only mock virtual methods
    MOCK_METHOD(RecoveryPolicy, GetPolicy, (), ());
    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(ServiceControlAction, Evaluate, (std::shared_ptr<Service> service), ());
};

inline MockPolicyBase* Mock_PolicyBase = nullptr;


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


} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_POLICY_BASE_HPP_
