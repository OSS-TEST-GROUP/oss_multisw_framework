
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_MONITOR_HPP_

#include <gmock/gmock.h>

#include "monitor/ServiceMonitor.hpp"
#include "service/Service.hpp"


namespace ros2 {
namespace adsw_manager {
    
// Mock ServiceMonitor
class MockServiceMonitor : public ServiceMonitor {
public:
    MockServiceMonitor() : ServiceMonitor() {}
    ~MockServiceMonitor() override = default;

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(void, UpdateServiceState, (UpdateServiceInfo info), ());
    MOCK_METHOD(void, NotifyServiceState, (), ());
    MOCK_METHOD(std::shared_ptr<ROSServiceDiscovery>, GetServiceDiscovery, (), ());
    MOCK_METHOD(int32_t, NodeStarted, (std::string service, std::string node), ());
    MOCK_METHOD(int32_t, NodeStopped, (std::string service, std::string node), ());
    MOCK_METHOD(int32_t, NodeCount, (std::string service), ());
    MOCK_METHOD(void, UpdateServiceState, (), ());
    MOCK_METHOD(void, NotifyAdswStatus, (int32_t status_code, std::string status_msg), ());
    MOCK_METHOD(void, OnContainerStateChanged,
                (const std::string& container_name, bool is_running, int32_t exit_code), ());
    MOCK_METHOD(void, OnServiceProcessDied,
                (const std::string& service_name, int32_t exit_code), ());

};

MockServiceMonitor* Mock_ServiceMonitor;

ServiceMonitor::ServiceMonitor() {
}
// ~ServiceMonitor() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.
bool ServiceMonitor::Initialize() {
    return Mock_ServiceMonitor->Initialize();
}
void ServiceMonitor::Finalize() {
    Mock_ServiceMonitor->Finalize();
}
void ServiceMonitor::UpdateServiceState(UpdateServiceInfo& info) {
    Mock_ServiceMonitor->UpdateServiceState(info);
}
void ServiceMonitor::NotifyServiceState() {
    Mock_ServiceMonitor->NotifyServiceState();
}
std::shared_ptr<ROSServiceDiscovery> ServiceMonitor::GetServiceDiscovery() {
    return Mock_ServiceMonitor->GetServiceDiscovery();
}
int32_t ServiceMonitor::NodeStarted(const std::string& service, const std::string& node) {
    return Mock_ServiceMonitor->NodeStarted(service, node);
}
int32_t ServiceMonitor::NodeStopped(const std::string& service, const std::string& node) {
    return Mock_ServiceMonitor->NodeStopped(service, node);
}
int32_t ServiceMonitor::NodeCount(const std::string& service) {
    return Mock_ServiceMonitor->NodeCount(service);
}
void ServiceMonitor::UpdateServiceState() {
    Mock_ServiceMonitor->UpdateServiceState();
}
void ServiceMonitor::NotifyAdswStatus(int32_t status_code, std::string status_msg) {
    Mock_ServiceMonitor->NotifyAdswStatus(status_code, status_msg);
}
void ServiceMonitor::OnContainerStateChanged(const std::string& container_name,
                                             bool is_running, int32_t exit_code) {
    Mock_ServiceMonitor->OnContainerStateChanged(container_name, is_running, exit_code);
}
// F013: process-death relay (ContainerHealthMonitor → ServiceMonitor)
void ServiceMonitor::OnServiceProcessDied(const std::string& service_name, int32_t exit_code) {
    if (Mock_ServiceMonitor) Mock_ServiceMonitor->OnServiceProcessDied(service_name, exit_code);
}

}
}

#endif