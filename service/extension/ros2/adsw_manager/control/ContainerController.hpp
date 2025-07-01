#ifndef ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_CONTAINER_CONTROLLER_HPP_


namespace ros2 {
namespace adsw_manager {

class ContainerController {
  public:
    ContainerController();
    ~ContainerController();

    bool Initialize();
    void Finalize();

    bool StartContainer();
    bool StopContainer();

  private:
};

} // namespace adsw_manager
} // namespace ros2


#endif
