#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS2_NODE_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS2_NODE_HPP_

#include <iostream>
#include <string>
#include <vector>

#include "ROS2Node.hpp"

namespace ros2 {
namespace adsw_manager {

class ROS2Node {
  public:
    std::string namespace_name;
    std::string node_name;
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
