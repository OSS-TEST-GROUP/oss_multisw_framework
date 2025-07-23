#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS2_PARTICIPANT_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS2_PARTICIPANT_HPP_

#include <iostream>
#include <string>
#include <vector>

#include "ROS2Node.hpp"

namespace ros2 {
namespace adsw_manager {

class ROS2Participant {

  public:
    ROS2Participant() = default;
    ~ROS2Participant() = default;

    bool SetGuessService(std::string service){
      if (service_name_ != service){
        service_name_ = service;
        return true; // changed
      }
      return false; // keep 
    }
    std::string GuessService(){ return service_name_;}

    std::vector<ROS2Node> nodes;

  private :
    std::string service_name_;

};

} // namespace adsw_manager
} // namespace ros2

#endif
