#ifndef GENERAL_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_
#define GENERAL_ADSW_MANAGER_MONITOR_SERVICE_MONITOR_HPP_

#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "UpdateServiceInfo.hpp"

namespace general {
namespace adsw_manager {

class ServiceMonitor {
  public:
    ServiceMonitor();
    virtual ~ServiceMonitor() = default;
    bool Initialize();
    void Finalize();
    
    void UpdateServiceState(UpdateServiceInfo& info);

    void NotifyServiceState();
    void NotifyAdswStatus(int32_t status_code, std::string status_msg);

    int32_t NodeStarted(const std::string& service, const std::string& node);
    int32_t NodeStopped(const std::string& service, const std::string& node);
    int32_t NodeCount(const std::string& service);

    void UpdateServiceState();

    // Called by ContainerHealthMonitor when a container's running state changes.
    void OnContainerStateChanged(const std::string& container_name,
                                 bool is_running,
                                 int32_t exit_code);

    // Called by ContainerHealthMonitor when an individual service's process
    // (identified by stored PID) is detected dead inside a running container.
    // SIGKILL inside a healthy container is invisible to docker-inspect-based
    // monitoring; this path handles it via per-PID polling.
    void OnServiceProcessDied(const std::string& service_name, int32_t exit_code);

  private:
    std::map<std::string, std::vector<std::string>> ros_service_node_list_;
};

} // namespace adsw_manager
} // namespace general

#endif
