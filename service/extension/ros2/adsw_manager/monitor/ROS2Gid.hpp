#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS2_GID_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS2_GID_HPP_

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "ros2/Gid.hpp"

namespace ros2 {
namespace adsw_manager {

class ROS2Gid {

    public :
    static std::string convert_gid_to_string(rmw_dds_common::msg::Gid& gid) {
        std::stringstream string_buffer;

        const auto& data = gid.data();
        
        for (const auto val : data) {
            string_buffer << std::hex << std::setw(2) << std::setfill('0') << static_cast<int32_t>(val);
        }
        return string_buffer.str();
    }
};

} // namespace adsw_manager
} // namespace ros2

#endif



