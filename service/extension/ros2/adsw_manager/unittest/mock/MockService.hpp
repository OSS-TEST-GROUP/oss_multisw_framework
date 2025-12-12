#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_SERVICE_HPP_

#include <gmock/gmock.h>

#include "service/Service.hpp"
#include "service/ServiceState.hpp"
#include "service/ServiceType.hpp"
#include "service/RecoveryPolicy.hpp"

namespace ros2 {
namespace adsw_manager {
    
class MockService : public Service {
  public:
    MockService(std::string name) : Service(name, ServiceType::NORMAL, RecoveryPolicy::ALWAYS, false) {}
    ~MockService() override = default;

    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(ServiceType, GetServiceType, (), ());
    MOCK_METHOD(RecoveryPolicy, GetRecoveryPolicy, (), ());
    MOCK_METHOD(bool, IsEnabled, (), ());
    MOCK_METHOD(ServiceState, GetServiceState, (), ());
    MOCK_METHOD(void, SetServiceState, (ServiceState state), ());
    MOCK_METHOD(void, SetLaunchFile, (const std::string& launchfile), ());
    MOCK_METHOD(const std::string, GetLaunchFile, (), ());
    MOCK_METHOD(void, SetPackage, (const std::string& package), ());
    MOCK_METHOD(const std::string, GetPackage, (), ());
    MOCK_METHOD(void, SetRunningNodes, (int32_t count), ());
    MOCK_METHOD(int32_t, GetRunningNodes, (), ());
    MOCK_METHOD(int32_t, GetMaxNodes, (), ());
    MOCK_METHOD(void, SetPID, (uint64_t pid), ());
    MOCK_METHOD(uint64_t, GetPID, (), ());
};    

MockService* Mock_Service;

Service::Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable)
    : name_(name), service_type_(service_type), recovery_policy_(recovery_policy), is_enabled_(is_enable),
      service_state_(ServiceState::NOT_RUNNING), launch_file_(""), max_nodes_(0), current_nodes_(0), pid_(0){
}
Service::~Service() {
}

std::string Service::GetName() {
    // Return the stored name directly instead of forwarding to mock
    // This is essential for ServiceRegistry to work properly
    return name_;
}
ServiceType Service::GetServiceType() {
    return Mock_Service->GetServiceType();
}
RecoveryPolicy Service::GetRecoveryPolicy() {
    return Mock_Service->GetRecoveryPolicy();
}
bool Service::IsEnabled() {
    return Mock_Service->IsEnabled();
}
ServiceState Service::GetServiceState() {
    return Mock_Service->GetServiceState();
}
void Service::SetServiceState(ServiceState state) {
    Mock_Service->SetServiceState(state);
}
void Service::SetLaunchFile(const std::string& launchfile) {
    Mock_Service->SetLaunchFile(launchfile);
}

void Service::SetPackage(const std::string& package) {
    Mock_Service->SetPackage(package);
}
const std::string Service::GetPackage() {
    return Mock_Service->GetPackage();
}
void Service::SetRunningNodes(int32_t count) {
    Mock_Service->SetRunningNodes(count);
}
int32_t Service::GetRunningNodes() {
    return Mock_Service->GetRunningNodes();
}
int32_t Service::GetMaxNodes()  {
    return Mock_Service->GetMaxNodes();
}
void Service::SetPID(uint64_t pid) {
    Mock_Service->SetPID(pid);
}
uint64_t Service::GetPID()  {
    return Mock_Service->GetPID();
}

}
}

#endif