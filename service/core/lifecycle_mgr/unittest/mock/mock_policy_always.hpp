
#ifndef MOCK_LIFECYCLE_POLICY_ALWAYS_HPP_
#define MOCK_LIFECYCLE_POLICY_ALWAYS_HPP_


#include "policy/PolicyAlways.hpp"
#include <gmock/gmock.h>


namespace lifecycle_manager {

class MockPolicyAlways : public PolicyAlways {
public:
    MockPolicyAlways(RecoveryPolicy policy, std::string name) : PolicyAlways(policy, name) {}
    virtual ~MockPolicyAlways() {}

    MOCK_METHOD(ServiceControlAction, Evaluate, (std::shared_ptr<Service> service), (override));
};

MockPolicyAlways* Mock_PolicyAlways = nullptr;

PolicyAlways::PolicyAlways(RecoveryPolicy policy, std::string name) 
    : PolicyBase(policy, name) {
   // Mock_PolicyAlways = this;   
}
PolicyAlways::~PolicyAlways() {
    Mock_PolicyAlways = nullptr;
}

ServiceControlAction PolicyAlways::Evaluate(std::shared_ptr<Service> service) {
    return Mock_PolicyAlways->Evaluate(service);
}

}
 // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_POLICY_ALWAYS_HPP_
