#ifndef ROS2_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_

namespace ros2 {
namespace adsw_manager {
    

    enum class ServiceState {
        NOT_RUNNING,
        INITIALIZING,
        RUNNING,
        RUNNING_NO_RESPONSE,
        TERMINATING
    };    
} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_
