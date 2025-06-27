#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS2_PARTICIPANT_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS2_PARTICIPANT_HPP_

#include <iostream>
#include <string>
#include <vector>

#include "ROS2Node.hpp"

namespace ros2 {
namespace adsw_manager {

class ROS2Participant {
  public:
    std::vector<ROS2Node> nodes;
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
