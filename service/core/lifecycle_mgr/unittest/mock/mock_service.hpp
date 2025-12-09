#ifndef MOCK_LIFECYCLE_SERVICE_HPP_
#define MOCK_LIFECYCLE_SERVICE_HPP_

#include <gmock/gmock.h>
#include "service/Service.hpp"

namespace lifecycle_manager {
class MockService : public Service {
public:
    MockService() : Service(), recovered_count_(0) {}
    MockService(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const OsType os)
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
    MOCK_METHOD(OsType, GetOsType, (), ());
    MOCK_METHOD(int32_t, GetRecoveredCount, (), ());

private:
    int32_t recovered_count_;
};

inline MockService* Mock_Service = nullptr;


Service::Service() {}
Service::Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const OsType os) {}
Service::~Service() {}

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

OsType Service::GetOsType() {
    return Mock_Service->GetOsType();
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_SERVICE_HPP_