#include "ROSServiceDiscovery.hpp"
#include "ROS2Gid.hpp"
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

    const auto& node_entities_info = info.node_entities_info_seq();
    const std::string participant_gid_hex = ROS2Gid::convert_gid_to_string(info.participant_gid());

    if (node_entities_info.size() == 0){
        LOG_ERROR("No node entities info found in the discovery message.");
        return;
    }

    std::shared_ptr<ROS2Participant> participant = GetROSParticipant(participant_gid_hex);

    if (participant == nullptr){
        participant = std::make_shared<ROS2Participant>();
        ros_participants_[participant_gid_hex] = participant;
    }

    participant->nodes.clear();
    
    for (const auto& participant_info : node_entities_info) {

        std::string ns = participant_info.node_namespace().c_str();
        std::string name = participant_info.node_name().c_str();

        ROS2Node node(ns, name);

        participant->nodes.push_back(node);
    }

}

std::shared_ptr<ROS2Participant> ROSServiceDiscovery::GetROSParticipant(const std::string& participant_id) {
    if (participant_id.empty()) {
        return nullptr;
    }

    auto it = ros_participants_.find(participant_id);
    if (it != ros_participants_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace adsw_manager
} // namespace ros2
