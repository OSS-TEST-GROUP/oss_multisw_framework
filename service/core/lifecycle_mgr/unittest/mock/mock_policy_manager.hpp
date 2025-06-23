// Google Mock for PolicyManager
#pragma once
#include <gmock/gmock.h>
#include "policy/PolicyManager.hpp"

namespace lifecycle_manager {
class MockPolicyManager : public PolicyManager {
public:
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    // Add more mocks as needed
};

MockPolicyManager* Mock_PolicyManager;

PolicyManager::PolicyManager() {}
PolicyManager::~PolicyManager() {}


bool PolicyManager::Start() {
    return Mock_PolicyManager->Start();
}

void PolicyManager::Stop() {
    Mock_PolicyManager->Stop();
}
std::shared_ptr<PolicyBase> PolicyManager::GetPolicy(RecoveryPolicy policy) {
    return Mock_PolicyManager->GetPolicy(policy);
}

} // namespace lifecycle_manager
