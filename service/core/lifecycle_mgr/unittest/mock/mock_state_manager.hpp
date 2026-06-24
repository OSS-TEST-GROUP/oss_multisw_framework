#ifndef MOCK_LIFECYCLE_STATE_MANAGER_HPP_
#define MOCK_LIFECYCLE_STATE_MANAGER_HPP_

#include <gmock/gmock.h>
#include "state/StateManager.hpp"

namespace lifecycle_manager {

class MockStateManager : public StateManager {
public:
    MockStateManager(LifecycleManager& mgr) : StateManager(mgr) {}
    ~MockStateManager() = default;

    // Only mock virtual methods
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(bool, StartService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(void, StopService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(bool, SetServiceMode, (lifecyclemgr::SetServiceModeRequest* message), ());
    MOCK_METHOD(void, ProcessServiceHandling, (std::set<std::shared_ptr<Service>> service_list), ());
    MOCK_METHOD(void, OnStart, (lifecyclemgr::StartServiceResponse* message), ());
    MOCK_METHOD(void, OnStop, (lifecyclemgr::StopServiceResponse* message), ());
    MOCK_METHOD(void, RequestStopAll, (), ());

    // F013: container actuator relays + ctrl/* gate handlers
    MOCK_METHOD(void, OnStartContainer, (lifecyclemgr::StartContainerResponse*), ());
    MOCK_METHOD(void, OnStopContainer, (lifecyclemgr::StopContainerResponse*), ());
    MOCK_METHOD(void, HandleCtrlStartService, (lifecyclemgr::CtrlStartServiceRequest*), ());
    MOCK_METHOD(void, HandleCtrlStopService, (lifecyclemgr::CtrlStopServiceRequest*), ());
    MOCK_METHOD(void, HandleCtrlSetServiceMode, (lifecyclemgr::CtrlSetServiceModeRequest*), ());
    MOCK_METHOD(void, HandleCtrlStartContainer, (lifecyclemgr::CtrlStartContainerRequest*), ());
    MOCK_METHOD(void, HandleCtrlStopContainer, (lifecyclemgr::CtrlStopContainerRequest*), ());

    // F014: per-service recovery policy mutation relays
    MOCK_METHOD(void, OnSetRecoveryPolicy, (lifecyclemgr::SetRecoveryPolicyResponse*), ());
    MOCK_METHOD(void, HandleCtrlSetRecoveryPolicy, (lifecyclemgr::CtrlSetRecoveryPolicyRequest*), ());
};

inline MockStateManager* Mock_StateManager = nullptr;


// Implementations moved to .cpp file to avoid multiple definition errors.
StateManager::StateManager(lifecycle_manager::LifecycleManager& mgr) : ManagerBase(mgr) {}
StateManager::~StateManager() {}

bool StateManager::Start() {
    return Mock_StateManager->Start();
}

void StateManager::Stop() {
    Mock_StateManager->Stop();
}
bool StateManager::StartService(std::shared_ptr<Service> service) {
    return Mock_StateManager->StartService(service);
}

void StateManager::StopService(std::shared_ptr<Service> service) {
    Mock_StateManager->StopService(service);
}

bool StateManager::SetServiceMode(lifecyclemgr::SetServiceModeRequest* message) {
    return Mock_StateManager->SetServiceMode(message);
}
void StateManager::ProcessServiceHandling(std::set<std::shared_ptr<Service>> service_list) {
    Mock_StateManager->ProcessServiceHandling(service_list);
}
void StateManager::OnStart(lifecyclemgr::StartServiceResponse* message) {
    Mock_StateManager->OnStart(message);
}

void StateManager::OnStop(lifecyclemgr::StopServiceResponse* message) {
    Mock_StateManager->OnStop(message);
}

void StateManager::RequestStopAll() {
    if (Mock_StateManager) {
        Mock_StateManager->RequestStopAll();
        return;
    }
    // Default no-op to avoid performing real actions during unit tests
}

// F013: container actuator relays + ctrl/* gate handlers — mock-forwarded
// so LifecycleManagerTest links without dragging in the real StateManager.cpp.
void StateManager::OnStartContainer(lifecyclemgr::StartContainerResponse* m) {
    if (Mock_StateManager) Mock_StateManager->OnStartContainer(m);
}
void StateManager::OnStopContainer(lifecyclemgr::StopContainerResponse* m) {
    if (Mock_StateManager) Mock_StateManager->OnStopContainer(m);
}
void StateManager::HandleCtrlStartService(lifecyclemgr::CtrlStartServiceRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlStartService(m);
}
void StateManager::HandleCtrlStopService(lifecyclemgr::CtrlStopServiceRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlStopService(m);
}
void StateManager::HandleCtrlSetServiceMode(lifecyclemgr::CtrlSetServiceModeRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlSetServiceMode(m);
}
void StateManager::HandleCtrlStartContainer(lifecyclemgr::CtrlStartContainerRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlStartContainer(m);
}
void StateManager::HandleCtrlStopContainer(lifecyclemgr::CtrlStopContainerRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlStopContainer(m);
}
// F014: per-service recovery policy mutation relays — mock-forwarded so
// LifecycleManagerTest links without dragging in the real StateManager.cpp.
void StateManager::OnSetRecoveryPolicy(lifecyclemgr::SetRecoveryPolicyResponse* m) {
    if (Mock_StateManager) Mock_StateManager->OnSetRecoveryPolicy(m);
}
void StateManager::HandleCtrlSetRecoveryPolicy(lifecyclemgr::CtrlSetRecoveryPolicyRequest* m) {
    if (Mock_StateManager) Mock_StateManager->HandleCtrlSetRecoveryPolicy(m);
}
// PublishCtrlResponse is private; not directly called from LifecycleManager.cpp,
// but the symbol must be resolvable if StateManager.cpp ever gets linked into a
// test fixture.  Mock it as a thin no-op stub for safety.
bool StateManager::PublishCtrlResponse(MessageType, lifecyclemgr::ReturnCode) {
    return true;
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_STATE_MANAGER_HPP_