#include "ROSDiscoveryInfoEndpoint.hpp"
#include "monitor/ROSServiceDiscovery.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"
#include "logger/Log.hpp"

#include "ADSWManager.hpp"

namespace ros2 {
namespace adsw_manager {
    
ROSDiscoveryInfoEndpoint::ROSDiscoveryInfoEndpoint(MessageType msgtype, std::string topic) 
    : DDSMessageEndpoint() {
        
    topic_ = topic;
    if (msgtype == MessageType::ROS2_DISCOVREY_INFO)
    {
        type_support_ = std::make_shared<TypeSupport>(new rmw_dds_common::msg::ParticipantEntitiesInfoPubSubType());
    }
}

ROSDiscoveryInfoEndpoint::~ROSDiscoveryInfoEndpoint() {}

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
    
    if (Read<rmw_dds_common::msg::ParticipantEntitiesInfo>(info) == true){
        LOG_DEBUG("ROSDiscoveryInfoEndpoint " << GetTopic() );

        std::shared_ptr<ROSServiceDiscovery> manager = ADSWManager::Get()->GetServiceDiscovery();
        if (manager != nullptr){
            manager->ParseROSDiscoveryInfo(info);
        }
        LOG_DEBUG("ROSDiscoveryInfoEndpoint processed " );

    }
    else {
        LOG_ERROR("Failed to read data from " << GetTopic());
    }
    return;
}


} // namespace adsw_manager
} // namespace ros2
