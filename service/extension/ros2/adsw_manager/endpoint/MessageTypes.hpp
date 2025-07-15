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
        STOP_SERVICE_RESPONSE
    };

    
} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_MESSAGE_TYPES_HPP_
