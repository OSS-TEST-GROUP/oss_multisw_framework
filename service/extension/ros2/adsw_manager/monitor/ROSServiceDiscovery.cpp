#include "ROSServiceDiscovery.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ROSServiceDiscovery::ROSServiceDiscovery() {}

ROSServiceDiscovery::~ROSServiceDiscovery() {}

bool ROSServiceDiscovery::Initialize() {
    LOG_DEBUG("ROSServiceDiscovery::Initialize ");
    return true;
}

void ROSServiceDiscovery::Finalize() {
    ros_participants_.clear();
}

void ROSServiceDiscovery::ParseROSDiscoveryInfo(rmw_dds_common::msg::ParticipantEntitiesInfo& info) {

    if (info.node_entities_info_seq().size() == 0){
        LOG_ERROR("No node entities info found in the discovery message.");
        return;
    }
    
    for (const auto& participant_info : info.node_entities_info_seq()) {
    }

}

ROS2Participant* ROSServiceDiscovery::GetROSParticipant(const std::string& participant_id) {
    if (participant_id.empty()) {
        return nullptr;
    }

    auto it = ros_participants_.find(participant_id);
    if (it != ros_participants_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace adsw_manager
} // namespace ros2
