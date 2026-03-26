#ifndef ROS2_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_

#include <iostream>
#include <string>
#include <memory>

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
    
    void UpdateServiceState(UpdateServiceInfo& info);

    void NotifyServiceState();
    void NotifyAdswStatus(int32_t status_code, std::string status_msg);

    std::shared_ptr<ROSServiceDiscovery> GetServiceDiscovery();

    int32_t NodeStarted(std::string service, std::string node);
    int32_t NodeStopped(std::string service, std::string node);
    int32_t NodeCount(std::string service);

    void UpdateServiceState();

    // Called by ContainerHealthMonitor when a container's running state changes.
    void OnContainerStateChanged(const std::string& container_name,
                                 bool is_running,
                                 int32_t exit_code);

  private:
    std::shared_ptr<ROSServiceDiscovery> ros_service_discovery_;
    std::map<std::string, std::vector<std::string>> ros_service_node_list_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
