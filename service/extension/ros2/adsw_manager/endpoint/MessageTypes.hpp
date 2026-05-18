#ifndef ROS2_ADSW_MANAGER_MESSAGE_TYPES_HPP_
#define ROS2_ADSW_MANAGER_MESSAGE_TYPES_HPP_

namespace ros2 {
namespace adsw_manager {
    

    enum class MessageType {
        ROS2_DISCOVREY_INFO,
        NOTIFY_SERVICE_STATE,

        UPDATE_SERVICE_LIST_REQUEST,
        UPDATE_SERVICE_LIST_RESPONSE,

        GET_SERVICE_STATE_REQUEST,
        GET_SERVICE_STATE_RESPONSE,

        START_SERVICE_REQUEST,
        START_SERVICE_RESPONSE,

        STOP_SERVICE_REQUEST,
        STOP_SERVICE_RESPONSE,

        ADSW_SERVICE_STATUS,

        // F013: container actuator surface (paired with lifecycle_mgr S2b)
        NOTIFY_CONTAINER_STATE,
        START_CONTAINER_REQUEST,
        START_CONTAINER_RESPONSE,
        STOP_CONTAINER_REQUEST,
        STOP_CONTAINER_RESPONSE,

        // F014 follow-up: recovery policy actuator (lifecycle_mgr → adsw)
        SET_RECOVERY_POLICY_REQUEST,
        SET_RECOVERY_POLICY_RESPONSE,
    };

    
} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_MESSAGE_TYPES_HPP_
