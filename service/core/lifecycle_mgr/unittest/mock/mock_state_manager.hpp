#ifndef MOCK_LIFECYCLE_STATE_MANAGER_HPP_
#define MOCK_LIFECYCLE_STATE_MANAGER_HPP_

#include <gmock/gmock.h>
#include "state/StateManager.hpp"

namespace lifecycle_manager {
class MockStateManager : public StateManager {
public:
    MockStateManager(LifecycleManager& mgr) : StateManager(mgr) {}
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(bool, StartService, (std::shared_ptr<Service> service), ());    
    MOCK_METHOD(void, StopService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(bool, SetServiceMode, (::SetServiceModeRequest* message), ());
    MOCK_METHOD(void, ProcessServiceHandling, (std::set<std::shared_ptr<Service>> service_list), ());
    MOCK_METHOD(void, OnStart, (::StartServiceResponse* message), ());
    MOCK_METHOD(void, OnStop, (::StopServiceResponse* message), ());    
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
bool StateManager::StartService(std::shared_ptr<Service> service) {
    return Mock_StateManager->StartService(service);
}

void StateManager::StopService(std::shared_ptr<Service> service) {
    Mock_StateManager->StopService(service);
}

bool StateManager::SetServiceMode(::SetServiceModeRequest* message) {
    return Mock_StateManager->SetServiceMode(message);
}
void StateManager::ProcessServiceHandling(std::set<std::shared_ptr<Service>> service_list) {
    Mock_StateManager->ProcessServiceHandling(service_list);
}
void StateManager::OnStart(::StartServiceResponse* message) {
    Mock_StateManager->OnStart(message);
}

void StateManager::OnStop(::StopServiceResponse* message) {
    Mock_StateManager->OnStop(message);
}


} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_STATE_MANAGER_HPP_