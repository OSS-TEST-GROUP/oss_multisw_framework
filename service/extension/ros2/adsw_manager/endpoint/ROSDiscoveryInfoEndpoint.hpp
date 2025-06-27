#ifndef ROS2_ADSW_MANAGER_ENDPOINT_ROS_DISCOVERY_INFO_ENDPOINT_HPP_
#define ROS2_ADSW_MANAGER_ENDPOINT_ROS_DISCOVERY_INFO_ENDPOINT_HPP_

#include <iostream>
#include <string>
#include <vector>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "ros2/ParticipantEntitiesInfoPubSubTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using ROSDiscoveryInfoEndpointCallback = std::function<void(rmw_dds_common::msg::ParticipantEntitiesInfo&)>;

class ROSDiscoveryInfoEndpoint : public DDSMessageEndpoint {
  public:
    ROSDiscoveryInfoEndpoint(ROSDiscoveryInfoEndpointCallback cb);
    virtual ~ROSDiscoveryInfoEndpoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();
  
  private:
    std::shared_ptr<TypeSupport> type_support_;
    ROSDiscoveryInfoEndpointCallback cb_;
};

} // namespace diagnostic_manager
} // namespace ros2

#endif
