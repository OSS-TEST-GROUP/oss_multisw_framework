#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_

#include <iostream>
#include <string>
#include <map>

#include "ROS2Participant.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"


namespace ros2 {
namespace adsw_manager {


class ROSServiceDiscovery {
  public:
    ROSServiceDiscovery();
    virtual ~ROSServiceDiscovery();

    bool Initialize();
    void Finalize();

    void ParseROSDiscoveryInfo(rmw_dds_common::msg::ParticipantEntitiesInfo& info);
    ROS2Participant* GetROSParticipant(const std::string& participant_id);

  private:
    std::map<std::string, ROS2Participant> ros_participants_;


};

} // namespace adsw_manager
} // namespace ros2

#endif
