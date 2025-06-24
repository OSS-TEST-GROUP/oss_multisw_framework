#include "ROSDiscoveryInfoEndpoint.hpp"


namespace ros2 {
namespace adsw_manager {
    
ROSDiscoveryInfoEndpoint::ROSDiscoveryInfoEndpoint(ROSDiscoveryInfoEndpointCallback cb) : cb_(cb) {
    type_support_ = std::make_shared<TypeSupport>(new rmw_dds_common::msg::ParticipantEntitiesInfoPubSubType());
}

ROSDiscoveryInfoEndpoint::~ROSDiscoveryInfoEndpoint() {}

std::string ROSDiscoveryInfoEndpoint::GetTopic() {
    return "ros_discovery_info";
}

std::string ROSDiscoveryInfoEndpoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> ROSDiscoveryInfoEndpoint::GetTypeSupport() {
    return type_support_;
}

void ROSDiscoveryInfoEndpoint::OnDataAvailable() {
    rmw_dds_common::msg::ParticipantEntitiesInfo info;
    Read<rmw_dds_common::msg::ParticipantEntitiesInfo>(info);

    if (cb_) {
        cb_(info);
    }
    return;
}


} // namespace diagnostic_manager
} // namespace ros2
