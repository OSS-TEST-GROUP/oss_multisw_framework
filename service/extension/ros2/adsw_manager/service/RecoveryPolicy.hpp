#ifndef ROS2_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_

namespace ros2 {
namespace adsw_manager {

    enum class RecoveryPolicy {
        R_DEFAULT,
        ALWAYS,
        ON_FAILURE,
        ON_SUCCESS,
        ON_NOT_RESPONDING,
        MANUAL
    };

} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_
