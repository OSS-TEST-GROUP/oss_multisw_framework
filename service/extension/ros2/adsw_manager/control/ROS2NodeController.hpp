#ifndef ROS2_ADSW_MANAGER_CONTROL_ROS2NODE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROS2NODE_CONTROLLER_HPP_

#include <string>

namespace ros2 {
namespace adsw_manager {

class ROS2NodeController {
  public:

    static bool StartNode(const std::string& basecommand, const std::string& package, const std::string& nodename);
    static bool StopNode(const std::string& basecommand, const std::string& package, const std::string& nodename);
    static bool GetState(const std::string& basecommand, const std::string& package, const std::string& nodename);
    static bool SetState(const std::string& basecommand, const std::string& package, const std::string& nodename, const std::string& state);
    static bool RestartNode(const std::string& basecommand, const std::string& package, const std::string& nodename);


  private:
};

} // namespace adsw_manager
} // namespace ros2

#endif