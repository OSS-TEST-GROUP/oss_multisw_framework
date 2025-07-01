#include "ServiceMonitor.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ServiceMonitor::ServiceMonitor() {}

ServiceMonitor::~ServiceMonitor() {}

bool ServiceMonitor::Initialize() {
    LOG_DEBUG("ServiceMonitor::Initialize start ");
    ros_service_discovery_ = std::make_shared<ROSServiceDiscovery>();

    if (ros_service_discovery_){
        ros_service_discovery_->Initialize();
    }
    LOG_DEBUG("ServiceMonitor::Initialize end ");
    return true;
}

void ServiceMonitor::Finalize() {
}

void ServiceMonitor::UpdateServiceState(UpdateServiceInfo info) {}

void ServiceMonitor::NotifyServiceState() {}


std::shared_ptr<ROSServiceDiscovery> ServiceMonitor::GetServiceDiscovery(){
    return ros_service_discovery_;
}

} // namespace adsw_manager
} // namespace ros2
