#ifndef ROS2_ADSW_MANAGER_CONTROL_ROS2NODE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROS2NODE_CONTROLLER_HPP_


namespace ros2 {
namespace adsw_manager {

class ROS2NodeController {
  public:
    ROS2NodeController();
    ~ROS2NodeController();

    bool Initialize();
    void Finalize();

    bool StartROS2Node();
    bool StopNode();

  private:
};

} // namespace adsw_manager
} // namespace ros2

#endif