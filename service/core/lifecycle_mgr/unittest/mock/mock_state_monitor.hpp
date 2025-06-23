// Google Mock for StateMonitor
#pragma once
#include <gmock/gmock.h>
#include "state/StateMonitor.hpp"

namespace lifecycle_manager {
class MockStateMonitor : public StateMonitor {
public:
    MockStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    // Add more mocks as needed
};

MockStateMonitor* Mock_StateMonitor;

StateMonitor::StateMonitor(lifecycle_manager::LifecycleManager& mgr) : ManagerBase(mgr) {}
StateMonitor::~StateMonitor() {}

bool StateMonitor::Start() {
    return Mock_StateMonitor->Start();
}

void StateMonitor::Stop() {
    Mock_StateMonitor->Stop();
}

} // namespace lifecycle_manager
