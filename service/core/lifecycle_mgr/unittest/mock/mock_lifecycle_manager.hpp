#ifndef MOCK_LIFECYCLE_LIFECYCLE_MANAGER_HPP_
#define MOCK_LIFECYCLE_LIFECYCLE_MANAGER_HPP_

#include <gmock/gmock.h>
#include "manager/LifecycleManager.hpp"

namespace lifecycle_manager {

class MockLifecycleManager : public LifecycleManager {
public:
    // Only mock virtual methods
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
    MOCK_METHOD(EndPointFactory*, GetEndpointFactory, (), ());
    MOCK_METHOD(std::shared_ptr<StateManager>, GetStateMgr, (), ());
    MOCK_METHOD(std::shared_ptr<StateMonitor>, GetMonitor, (), ());
    MOCK_METHOD(std::shared_ptr<PolicyManager>, GetPolicy, (), ());
    MOCK_METHOD(std::shared_ptr<MWServiceManager>, GetMWService, (), ());
    MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetRegistry, (), ());
    MOCK_METHOD(std::shared_ptr<Service>, GetService, (std::string name), ());
};

inline MockLifecycleManager* Mock_LifecycleManager = nullptr;


LifecycleManager::LifecycleManager() {}
// LifecycleManager::~LifecycleManager() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.


bool LifecycleManager::Initialize() {
    return Mock_LifecycleManager->Initialize();
}

bool LifecycleManager::Start() {
    return Mock_LifecycleManager->Start();
}

void LifecycleManager::Stop() {
    Mock_LifecycleManager->Stop();
}

std::shared_ptr<DDSTransmitter> LifecycleManager::GetTransmitter() {
    return Mock_LifecycleManager->GetTransmitter();
}

EndPointFactory* LifecycleManager::GetEndpointFactory() {
    return Mock_LifecycleManager->GetEndpointFactory();
}

std::shared_ptr<StateManager> LifecycleManager::GetStateMgr() {
    return Mock_LifecycleManager->GetStateMgr();
}

std::shared_ptr<StateMonitor> LifecycleManager::GetMonitor() {
    return Mock_LifecycleManager->GetMonitor();
}

std::shared_ptr<PolicyManager> LifecycleManager::GetPolicy() {
    return Mock_LifecycleManager->GetPolicy();
}

std::shared_ptr<MWServiceManager> LifecycleManager::GetMWService() {
    return Mock_LifecycleManager->GetMWService();
}

std::shared_ptr<ServiceRegistry> LifecycleManager::GetRegistry() {
    return Mock_LifecycleManager->GetRegistry();
}

std::shared_ptr<Service> LifecycleManager::GetService(std::string name) {
    return Mock_LifecycleManager->GetService(name);
}

void LifecycleManager::SetState(ManagerState state){
    Mock_LifecycleManager->SetState(state);
}


} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_LIFECYCLE_MANAGER_HPP_