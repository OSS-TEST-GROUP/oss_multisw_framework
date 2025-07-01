#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS2_NODE_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS2_NODE_HPP_

#include <iostream>
#include <string>
#include <vector>


namespace ros2 {
namespace adsw_manager {

class ROS2Node {
  
  public:
    ROS2Node(const std::string& ns_name = "", const std::string& n_name = "")
      : namespace_name(ns_name), node_name(n_name) {}

    std::string namespace_name;
    std::string node_name;
};

} // namespace adsw_manager
} // namespace ros2

#endif
