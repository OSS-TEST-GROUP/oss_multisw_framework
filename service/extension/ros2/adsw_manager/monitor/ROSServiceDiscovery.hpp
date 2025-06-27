#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_

#include <iostream>
#include <string>

namespace ros2 {
namespace adsw_manager {

class ROSServiceDiscovery {
  public:
    ROSServiceDiscovery();
    virtual ~ROSServiceDiscovery();

    bool Initialize();
    void Finalize();

    void ParseROSDiscoveryInfo();
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
