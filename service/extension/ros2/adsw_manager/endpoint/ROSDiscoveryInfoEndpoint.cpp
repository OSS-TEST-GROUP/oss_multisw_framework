#include "ROSDiscoveryInfoEndpoint.hpp"

#include "logger/Log.hpp"
#include "monitor/ROSServiceDiscovery.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"
#include <utility>

#include "ADSWManager.hpp"

namespace ros2 {
namespace adsw_manager {

ROSDiscoveryInfoEndpoint::ROSDiscoveryInfoEndpoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::ROS2_DISCOVREY_INFO)
    {
        type_support_ = std::make_shared<TypeSupport>(new rmw_dds_common::msg::ParticipantEntitiesInfoPubSubType());
    }
}


std::string ROSDiscoveryInfoEndpoint::GetTopic() {
    return topic_;
}

std::string ROSDiscoveryInfoEndpoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> ROSDiscoveryInfoEndpoint::GetTypeSupport() {
    return type_support_;
}

void ROSDiscoveryInfoEndpoint::OnDataAvailable() {
    rmw_dds_common::msg::ParticipantEntitiesInfo info;

    if (Read<rmw_dds_common::msg::ParticipantEntitiesInfo>(info)) {
        LOG_DEBUG("ROSDiscoveryInfoEndpoint " << GetTopic() );

        std::shared_ptr<ROSServiceDiscovery> manager = ADSWManager::Get()->GetServiceDiscovery();
        if (manager != nullptr){
            manager->ParseROSDiscoveryInfo(info);
        }
        LOG_DEBUG("ROSDiscoveryInfoEndpoint processed " );

    } else {
        LOG_ERROR("Failed to read data from " << GetTopic());
    }
}

} // namespace adsw_manager
} // namespace ros2
