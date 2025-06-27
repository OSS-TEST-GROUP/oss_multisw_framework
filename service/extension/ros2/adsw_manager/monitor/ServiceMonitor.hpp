#ifndef ROS2_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_

#include <iostream>
#include <string>

#include "ROSServiceDiscovery.hpp"
#include "UpdateServiceInfo.hpp"

namespace ros2 {
namespace adsw_manager {

class ServiceMonitor {
  public:
    ServiceMonitor();
    virtual ~ServiceMonitor();

    bool Initialize();
    void Finalize();
    
    void UpdateServiceState(UpdateServiceInfo info);

    void NotifyServiceState();

  private:
    std::shared_ptr<ROSServiceDiscovery> ros_service_discovery_;
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
