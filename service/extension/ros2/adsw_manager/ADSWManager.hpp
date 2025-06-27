#ifndef ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_

namespace ros2 {
namespace adsw_manager {

class ADSWManager {
  public:
    ADSWManager();
    virtual ~ADSWManager();

    bool Initialize();

    bool Start();
    void Stop();

  private:
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
