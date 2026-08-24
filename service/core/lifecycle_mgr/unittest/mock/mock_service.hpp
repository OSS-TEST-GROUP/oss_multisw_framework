#ifndef MOCK_LIFECYCLE_SERVICE_HPP_
#define MOCK_LIFECYCLE_SERVICE_HPP_

#include <gmock/gmock.h>
#include "service/Service.hpp"

namespace lifecycle_manager {
class MockService : public Service {
public:
    MockService() : Service(), recovered_count_(0) {}
    MockService(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const MwType os)
        : Service(name, type, policy, enabled, os), recovered_count_(0) {}
    ~MockService() = default;

    // Only mock virtual methods
    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(ServiceType, GetServiceType, (), ());
    MOCK_METHOD(RecoveryPolicy, GetRecoveryPolicy, (), ());
    MOCK_METHOD(bool, isEnabled, (), ());
    MOCK_METHOD(ServiceMode, GetServiceMode, (), ());
    MOCK_METHOD(void, SetServiceMode, (ServiceMode), ());
    MOCK_METHOD(ServiceState, GetServiceState, (), ());
    MOCK_METHOD(void, SetServiceState, (ServiceState), ());
    MOCK_METHOD(ServiceState, GetLastServiceState, (), ());
    MOCK_METHOD(void, Print, (), ());
    MOCK_METHOD(MwType, GetMwType, (), ());
    MOCK_METHOD(int32_t, GetRecoveredCount, (), ());
    MOCK_METHOD(int32_t, GetExitCode, (), ());
    MOCK_METHOD(void, SetExitCode, (int32_t), ());
    MOCK_METHOD(void, SetServiceType, (ServiceType), ());
    MOCK_METHOD(void, SetRecoveryPolicy, (RecoveryPolicy), ());
    MOCK_METHOD(void, SetEnabled, (bool), ());

private:
    int32_t recovered_count_;
};

inline MockService* Mock_Service = nullptr;


Service::Service() {}
Service::Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const MwType os) {}
// Service::~Service() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

bool Service::isEnabled() {
    return Mock_Service->isEnabled();
}

ServiceType Service::GetServiceType() {
    return Mock_Service->GetServiceType();
}

RecoveryPolicy Service::GetRecoveryPolicy() {
    return Mock_Service->GetRecoveryPolicy();
}


std::string Service::GetName() {
    return Mock_Service->GetName();
}

ServiceMode Service::GetServiceMode() {
    return Mock_Service->GetServiceMode();
}

void Service::SetServiceMode(ServiceMode mode) {
    Mock_Service->SetServiceMode(mode);
}

ServiceState Service::GetServiceState() {
    return Mock_Service->GetServiceState();
}

void Service::SetServiceState(ServiceState state) {
    Mock_Service->SetServiceState(state);
}

ServiceState Service::GetLastServiceState() {
    return Mock_Service->GetLastServiceState();
}

int32_t Service::GetRecoveredCount() {
    return Mock_Service->GetRecoveredCount();
}

void Service::IncreaseRecoveredCount() {
    Mock_Service->IncreaseRecoveredCount();
}

void Service::ClearRecoveredCount() {
    Mock_Service->ClearRecoveredCount();
}

void Service::Print() {
    Mock_Service->Print();
}

MwType Service::GetMwType() {
    return Mock_Service->GetMwType();
}

int32_t Service::GetExitCode() {
    return Mock_Service->GetExitCode();
}

void Service::SetExitCode(int32_t code) {
    Mock_Service->SetExitCode(code);
}

void Service::SetServiceType(ServiceType type) {
    Mock_Service->SetServiceType(type);
}

void Service::SetRecoveryPolicy(RecoveryPolicy policy) {
    Mock_Service->SetRecoveryPolicy(policy);
}

void Service::SetEnabled(bool enabled) {
    Mock_Service->SetEnabled(enabled);
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_SERVICE_HPP_