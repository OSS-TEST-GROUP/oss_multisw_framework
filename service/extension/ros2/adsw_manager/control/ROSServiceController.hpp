#ifndef ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_

#include <string>


namespace ros2 {
namespace adsw_manager {

class ROSServiceController  {

  public:
    ROSServiceController();
    ~ROSServiceController();

    bool Initialize();
    void Finalize();

    bool StartGroup();
    bool StopGroup();

    bool StartService(const std::string& service_name);
    bool StopService(const std::string& service_name);

  private:
};

} // namespace adsw_manager
} // namespace ros2

#endif
