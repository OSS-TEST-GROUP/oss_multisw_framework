#include "Service.hpp"

namespace ros2 {
namespace adsw_manager {

Service::Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable)
    : name_(name),
      service_type_(service_type),
      recovery_policy_(recovery_policy),
      is_enabled_(is_enable),
      service_state_(ServiceState::NOT_RUNNING)
{
}

Service::~Service() {}

std::string Service::GetName() {
    return name_;
}

ServiceType Service::GetServiceType() {
    return service_type_;
}

RecoveryPolicy Service::GetRecoveryPolicy() {
    return recovery_policy_;
}

bool Service::IsEnabled() {
    return is_enabled_;
}

ServiceState Service::GetServiceState() {
    return service_state_;
}

void Service::SetServiceState(ServiceState state) {
    service_state_ = state;
}

} // namespace adsw_manager
} // namespace ros2