
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_MONITOR_HPP_

#include <gmock/gmock.h>

#include "monitor/ServiceMonitor.hpp"
#include "service/Service.hpp"


using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    
// Mock ServiceMonitor
class MockServiceMonitor : public ServiceMonitor {
public:
    MockServiceMonitor() : ServiceMonitor() {}
    virtual ~MockServiceMonitor() {}

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(void, UpdateServiceState, (UpdateServiceInfo info), ());
    MOCK_METHOD(void, NotifyServiceState, (), ());
    MOCK_METHOD(std::shared_ptr<ROSServiceDiscovery>, GetServiceDiscovery, (), ());
    MOCK_METHOD(int32_t, NodeStarted, (std::string service, std::string node), ());
    MOCK_METHOD(int32_t, NodeStopped, (std::string service, std::string node), ());
    MOCK_METHOD(int32_t, NodeCount, (std::string service), ());
    MOCK_METHOD(void, UpdateServiceState, (), ());

};

MockServiceMonitor* Mock_ServiceMonitor;

ServiceMonitor::ServiceMonitor() {
    Mock_ServiceMonitor = new MockServiceMonitor();
}
ServiceMonitor::~ServiceMonitor() {
    Mock_ServiceMonitor = nullptr;
}
bool ServiceMonitor::Initialize() {
    return Mock_ServiceMonitor->Initialize();
}
void ServiceMonitor::Finalize() {
    Mock_ServiceMonitor->Finalize();
}
void ServiceMonitor::UpdateServiceState(UpdateServiceInfo info) {
    Mock_ServiceMonitor->UpdateServiceState(info);
}
void ServiceMonitor::NotifyServiceState() {
    Mock_ServiceMonitor->NotifyServiceState();
}
std::shared_ptr<ROSServiceDiscovery> ServiceMonitor::GetServiceDiscovery() {
    return Mock_ServiceMonitor->GetServiceDiscovery();
}
int32_t ServiceMonitor::NodeStarted(std::string service, std::string node) {
    return Mock_ServiceMonitor->NodeStarted(service, node);
}
int32_t ServiceMonitor::NodeStopped(std::string service, std::string node) {
    return Mock_ServiceMonitor->NodeStopped(service, node);
}
int32_t ServiceMonitor::NodeCount(std::string service) {
    return Mock_ServiceMonitor->NodeCount(service);
}
void ServiceMonitor::UpdateServiceState() {
    Mock_ServiceMonitor->UpdateServiceState();
}



}
}

#endif