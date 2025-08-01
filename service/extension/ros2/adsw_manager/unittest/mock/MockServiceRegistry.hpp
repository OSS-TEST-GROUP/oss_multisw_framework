
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_REGISTRY_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_REGISTRY_HPP_

#include <gmock/gmock.h>

#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    
// Simple mock ServiceRegistry
class MockServiceRegistry : public ServiceRegistry {
public:
    MockServiceRegistry() : ServiceRegistry() {}
    virtual ~MockServiceRegistry() {}

    MOCK_METHOD(bool, LoadServiceList, (), ());
    MOCK_METHOD(void, AddGroup, (const std::string& name), ());
    MOCK_METHOD(void, RemoveGroup, (const std::string& name), ());
    MOCK_METHOD(bool, IsGroup, (const std::string& name), ());
    MOCK_METHOD(bool, AddService, (std::shared_ptr<Service> service, const std::string& group), ());
    MOCK_METHOD(void, RemoveService, (const std::string& name), ());
    MOCK_METHOD(std::shared_ptr<Service>, GetService, (const std::string& name), ());
    MOCK_METHOD(std::vector<std::shared_ptr<Service>>, GetServicesByGroup, (const std::string& group), ());
    MOCK_METHOD(std::string, GetGroupNameByService, (std::shared_ptr<Service> service), ());
    MOCK_METHOD(uint64_t, GetServicesNum, (), ());
    MOCK_METHOD(uint64_t, GetServiceNumInGroup, (const std::string& group), ());
    MOCK_METHOD(uint64_t, GetGroupNum, (), ());
    MOCK_METHOD(bool, ProcessGetServiceStateRequest, (lifecyclemgr::GetServiceStateRequest* message), ());
    MOCK_METHOD(bool, LoadServiceList, (const std::string& path), ());
    MOCK_METHOD(bool, ProcessUpdateServiceListRequest, (), ());
    MOCK_METHOD(bool, Save, (), ());
    MOCK_METHOD(bool, LoadServiceSaved, (const std::string& path), ());


};

MockServiceRegistry* Mock_ServiceRegistry;

ServiceRegistry::ServiceRegistry() {
    Mock_ServiceRegistry = new MockServiceRegistry();
}
ServiceRegistry::~ServiceRegistry() {
    Mock_ServiceRegistry = nullptr;
}
bool ServiceRegistry::LoadServiceList() {
    return Mock_ServiceRegistry->LoadServiceList();
}
void ServiceRegistry::AddGroup(const std::string& name) {
    Mock_ServiceRegistry->AddGroup(name);
}
void ServiceRegistry::RemoveGroup(const std::string& name) {
    Mock_ServiceRegistry->RemoveGroup(name);
}
bool ServiceRegistry::IsGroup(const std::string& name) {
    return Mock_ServiceRegistry->IsGroup(name);
}
bool ServiceRegistry::AddService(std::shared_ptr<Service> service, const std::string& group) {
    return Mock_ServiceRegistry->AddService(service, group);
}
void ServiceRegistry::RemoveService(const std::string& name) {
    Mock_ServiceRegistry->RemoveService(name);
}
std::shared_ptr<Service> ServiceRegistry::GetService(const std::string& name) {
    return Mock_ServiceRegistry->GetService(name);
}
std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServicesByGroup(const std::string& group) {
    return Mock_ServiceRegistry->GetServicesByGroup(group);
}
std::string ServiceRegistry::GetGroupNameByService(std::shared_ptr<Service> service) {
    return Mock_ServiceRegistry->GetGroupNameByService(service);
}
uint64_t ServiceRegistry::GetServicesNum() {
    return Mock_ServiceRegistry->GetServicesNum();
}
uint64_t ServiceRegistry::GetServiceNumInGroup(const std::string& group) {
    return Mock_ServiceRegistry->GetServiceNumInGroup(group);
}
uint64_t ServiceRegistry::GetGroupNum() {
    return Mock_ServiceRegistry->GetGroupNum();
}
bool ServiceRegistry::ProcessGetServiceStateRequest(lifecyclemgr::GetServiceStateRequest* message) {
    return Mock_ServiceRegistry->ProcessGetServiceStateRequest(message);
}
bool ServiceRegistry::LoadServiceList(const std::string& path) {
    return Mock_ServiceRegistry->LoadServiceList(path);
}

bool ServiceRegistry::ProcessUpdateServiceListRequest() {
    return Mock_ServiceRegistry->ProcessUpdateServiceListRequest();
}
bool ServiceRegistry::Save() {
    return Mock_ServiceRegistry->Save();
}
bool ServiceRegistry::LoadServiceSaved(const std::string& path) {
    return Mock_ServiceRegistry->LoadServiceSaved(path);
}



}
}

#endif