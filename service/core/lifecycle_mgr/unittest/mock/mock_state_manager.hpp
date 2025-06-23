// Google Mock for StateManager
#pragma once
#include <gmock/gmock.h>
#include "state/StateManager.hpp"

namespace lifecycle_manager {
class MockStateManager : public StateManager {
public:
    MockStateManager(LifecycleManager& mgr) : StateManager(mgr) {}
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    // Add more mocks as needed
};

MockStateManager* Mock_StateManager;

// Implementations moved to .cpp file to avoid multiple definition errors.
StateManager::StateManager(lifecycle_manager::LifecycleManager& mgr) : ManagerBase(mgr) {}
StateManager::~StateManager() {}

bool StateManager::Start() {
    return Mock_StateManager->Start();
}

void StateManager::Stop() {
    Mock_StateManager->Stop();
}

} // namespace lifecycle_manager
