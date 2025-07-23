#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include "ROS2Participant.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"


namespace ros2 {
namespace adsw_manager {

class ServiceMonitor;


class ROSServiceDiscovery {
  public:
    ROSServiceDiscovery(ServiceMonitor* service_monitor);
    virtual ~ROSServiceDiscovery();

    bool Initialize();
    void Finalize();

    void ParseROSDiscoveryInfo(rmw_dds_common::msg::ParticipantEntitiesInfo& info);
    std::shared_ptr<ROS2Participant> GetROSParticipant(const std::string& participant_id);

  private:
    std::string guessService(const std::string ns);

  private:
    ServiceMonitor* service_monitor_;
    std::map<std::string, std::shared_ptr<ROS2Participant>> ros_participants_;

    std::map<std::string, std::string> ros_group_participants_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
