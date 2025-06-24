#include "ServiceMonitor.hpp"

namespace ros2 {
namespace adsw_manager {

ServiceMonitor::ServiceMonitor() {}

ServiceMonitor::~ServiceMonitor() {}

bool ServiceMonitor::Initialize() {
    return true;
}

void ServiceMonitor::Finalize() {}

void ServiceMonitor::UpdateServiceState(UpdateServiceInfo info) {}

void ServiceMonitor::NotifyServiceState() {}


} // namespace diagnostic_manager
} // namespace ros2
