#pragma once

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

} // namespace lifecycle_manager
