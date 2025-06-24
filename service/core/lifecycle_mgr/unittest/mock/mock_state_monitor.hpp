#ifndef MOCK_LIFECYCLE_STATE_MONITOR_HPP_
#define MOCK_LIFECYCLE_STATE_MONITOR_HPP_

#include <gmock/gmock.h>
#include "state/StateMonitor.hpp"

namespace lifecycle_manager {
class MockStateMonitor : public StateMonitor {
public:
    MockStateMonitor(LifecycleManager& mgr) : StateMonitor(mgr) {}
    virtual ~MockStateMonitor() {}
    
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, UpdateServicesState, (::NotifyServiceState*), ());
    MOCK_METHOD(void, UpdateServicesState, (::GetServiceStateResponse*), ());
    MOCK_METHOD(void, SetUpdateStateFlag, (std::shared_ptr<Service>), ());
    MOCK_METHOD(void, ClearUpdateStateFlag, (std::shared_ptr<Service>), ());
    MOCK_METHOD(bool, GetUpdateStateFlag, (std::shared_ptr<Service>), ());
    MOCK_METHOD(void, ClearAllUpdatedFlag, (), ());
    MOCK_METHOD(std::set<std::shared_ptr<Service>>, GetUpdateServiceList, (), ());
    MOCK_METHOD(void, NotifyLifecycleDiagnosticInfo, (bool), ());
    MOCK_METHOD(void, StartThread, (), ());
    MOCK_METHOD(void, StopThread, (), ());
    MOCK_METHOD(void, RunThread, (), ());
    MOCK_METHOD(void, RequestGetState, (), ());
    MOCK_METHOD(void, updateServices, (std::vector<std::string>*, std::vector<::ServiceState>*), ());
    MOCK_METHOD(bool, getServiceState, (std::shared_ptr<Service>, ::ServiceStatus&), ());
    
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
void StateMonitor::UpdateServicesState(::NotifyServiceState* update) {
    Mock_StateMonitor->UpdateServicesState(update);
}

void StateMonitor::UpdateServicesState(::GetServiceStateResponse* update) {
    Mock_StateMonitor->UpdateServicesState(update);
}
void StateMonitor::SetUpdateStateFlag(std::shared_ptr<Service> service) {
    Mock_StateMonitor->SetUpdateStateFlag(service); 
}

void StateMonitor::ClearUpdateStateFlag(std::shared_ptr<Service> service) {
    Mock_StateMonitor->ClearUpdateStateFlag(service);
}
bool StateMonitor::GetUpdateStateFlag(std::shared_ptr<Service> service) {
    return Mock_StateMonitor->GetUpdateStateFlag(service);
}
void StateMonitor::ClearAllUpdatedFlag() {
    Mock_StateMonitor->ClearAllUpdatedFlag();
}
std::set<std::shared_ptr<Service>> StateMonitor::GetUpdateServiceList() {
    return Mock_StateMonitor->GetUpdateServiceList();
}
void StateMonitor::NotifyLifecycleDiagnosticInfo(bool is_enabled) {
    Mock_StateMonitor->NotifyLifecycleDiagnosticInfo(is_enabled);
}
void StateMonitor::StartThread() {
    Mock_StateMonitor->StartThread();
}
void StateMonitor::StopThread() {
    Mock_StateMonitor->StopThread();
}
void StateMonitor::RunThread() {
    Mock_StateMonitor->RunThread();
}
void StateMonitor::RequestGetState() {  
    Mock_StateMonitor->RequestGetState();
}
void StateMonitor::updateServices(std::vector<std::string>* slist, std::vector<::ServiceState>* statelist) {
    Mock_StateMonitor->updateServices(slist, statelist);

}
bool StateMonitor::getServiceState(std::shared_ptr<Service> service, ::ServiceStatus& entry) {
    return Mock_StateMonitor->getServiceState(service, entry);
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_STATE_MONITOR_HPP_
