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
    
    // Public fields for test data storage
    ServiceState service_state_ = ServiceState::NOT_RUNNING;
    int32_t running_nodes_ = 0;
};    

MockService* Mock_Service;

Service::Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable)
    : name_(name), service_type_(service_type), recovery_policy_(recovery_policy), is_enabled_(is_enable),
      service_state_(ServiceState::NOT_RUNNING), launch_file_(""), max_nodes_(0), current_nodes_(0), pid_(0),
      exit_code_(0), is_direct_exec_(false), is_container_workload_(false){
}
// ~Service() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

std::string Service::GetName() {
    // Return the stored name directly instead of forwarding to mock
    // This is essential for ServiceRegistry to work properly
    return name_;
}
ServiceType Service::GetServiceType() {
    if (Mock_Service) return Mock_Service->GetServiceType();
    return service_type_;
}
RecoveryPolicy Service::GetRecoveryPolicy() {
    if (Mock_Service) return Mock_Service->GetRecoveryPolicy();
    return recovery_policy_;
}
bool Service::IsEnabled() {
    if (Mock_Service) return Mock_Service->IsEnabled();
    return is_enabled_;
}
ServiceState Service::GetServiceState() {
    if (Mock_Service) return Mock_Service->GetServiceState();
    return service_state_;
}
void Service::SetServiceState(ServiceState state) {
    if (Mock_Service) Mock_Service->SetServiceState(state);
    service_state_ = state;
}
void Service::SetLaunchFile(const std::string& launchfile) {
    if (Mock_Service) Mock_Service->SetLaunchFile(launchfile);
    launch_file_ = launchfile;
}

void Service::SetPackage(const std::string& package) {
    if (Mock_Service) Mock_Service->SetPackage(package);
    packag_name_ = package;
}
std::string Service::GetPackage() {
    if (Mock_Service) return Mock_Service->GetPackage();
    return packag_name_;
}
void Service::SetRunningNodes(int32_t count) {
    if (Mock_Service) Mock_Service->SetRunningNodes(count);
    current_nodes_ = count;
}
int32_t Service::GetRunningNodes() {
    if (Mock_Service) return Mock_Service->GetRunningNodes();
    return current_nodes_;
}
int32_t Service::GetMaxNodes()  {
    if (Mock_Service) return Mock_Service->GetMaxNodes();
    return max_nodes_;
}
void Service::SetPID(uint64_t pid) {
    if (Mock_Service) Mock_Service->SetPID(pid);
    pid_ = pid;
}
uint64_t Service::GetPID()  {
    if (Mock_Service) return Mock_Service->GetPID();
    return pid_;
}

void Service::SetExitCode(int32_t code) {
    exit_code_ = code;
}
int32_t Service::GetExitCode() {
    return exit_code_;
}

// F013: process-match pattern + direct-exec accessors — backed by real members
// (mirrors Service.cpp) so ServiceRegistry/monitor link in unit tests.
void Service::SetProcessMatchPattern(const std::string& pattern) {
    match_pattern_ = pattern;
}
const std::string& Service::GetProcessMatchPattern() const {
    return match_pattern_;
}
void Service::SetDirectExec(bool direct) {
    is_direct_exec_ = direct;
}
bool Service::IsDirectExec() const {
    return is_direct_exec_;
}

// container-workload accessor — 위와 같은 이유로 실제 멤버를 그대로 쓴다.
// ServiceRegistry::Save / LoadServiceSaved 가 호출하므로 없으면
// ADSWServiceRegistryTest 가 링크되지 않는다.
void Service::SetContainerWorkload(bool v) {
    is_container_workload_ = v;
}
bool Service::IsContainerWorkload() const {
    return is_container_workload_;
}

}
}

#endif