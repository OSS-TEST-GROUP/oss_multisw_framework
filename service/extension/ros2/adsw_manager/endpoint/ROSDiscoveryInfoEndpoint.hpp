#ifndef ROS2_ADSW_MANAGER_ENDPOINT_ROS_DISCOVERY_INFO_ENDPOINT_HPP_
#define ROS2_ADSW_MANAGER_ENDPOINT_ROS_DISCOVERY_INFO_ENDPOINT_HPP_

#include <iostream>
#include <string>
#include <vector>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "ros2/ParticipantEntitiesInfoPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

class ROSDiscoveryInfoEndpoint : public DDSMessageEndpoint {
  public:
    ROSDiscoveryInfoEndpoint(MessageType msgtype, std::string topic);
    ~ROSDiscoveryInfoEndpoint() override= default;
    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
