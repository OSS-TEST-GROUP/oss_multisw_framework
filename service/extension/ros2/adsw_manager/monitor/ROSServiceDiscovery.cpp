#include <vector>
#include <algorithm>

#include "ROSServiceDiscovery.hpp"
#include "ROS2Gid.hpp"
#include "ServiceMonitor.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ROSServiceDiscovery::ROSServiceDiscovery(ServiceMonitor* service_monitor) 
    : service_monitor_(service_monitor), ros_service_discovery_initialized_(false) {

}


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

    std::shared_ptr<ROS2Participant> participant = GetROSParticipant(participant_gid_hex);
    std::string expect_service;

    if (participant != nullptr){
        expect_service = participant->GuessService();
    }

    if ((participant == nullptr) && (!node_entities_info.empty())) {
        LOG_DEBUG("ParseROSDiscoveryInfo - new participant " << participant_gid_hex << ", " << node_entities_info.size());
        participant = std::make_shared<ROS2Participant>();
        ros_participants_[participant_gid_hex] = participant;
    } else if ((participant != nullptr) && (node_entities_info.empty())) {
        LOG_DEBUG("ParseROSDiscoveryInfo - delete participant " << participant_gid_hex << ", " << node_entities_info.size());
    } else if ((participant == nullptr) && (node_entities_info.empty())) {
        LOG_DEBUG("ParseROSDiscoveryInfo - invalid participant " << participant_gid_hex << ", " << node_entities_info.size());
        return;
    } else {
        // participant != null && node_endty > 0
        LOG_DEBUG("ParseROSDiscoveryInfo - update node list " << participant_gid_hex << ", " << node_entities_info.size());
    }

    if (!ros_service_discovery_initialized_) {
        ros_service_discovery_initialized_ = true;
        LOG_DEBUG("ROSServiceDiscovery::ParseROSDiscoveryInfo - Initial Discovery Completed ");
        service_monitor_->NotifyAdswStatus(1, "ADSW is ready running");
    }

    if (participant != nullptr){
        std::vector<std::string> prev_node_lists_;
        std::vector<std::string> new_node_lists_;

        // save all node names to list;
        for (const auto& node : participant->nodes) {
            prev_node_lists_.push_back(node.node_name);
        }
        // reset for new node list
        participant->nodes.clear();

        for (const auto& participant_info : node_entities_info) {

            std::string ns = participant_info.node_namespace().c_str();
            std::string name = participant_info.node_name().c_str();

            ROS2Node node(ns, name);
        
            LOG_DEBUG("[ ns : " << ns << ", name : " << name << " ] " << participant->nodes.size());

            participant->nodes.push_back(node);
            if (expect_service.empty()){
                // do only empty case - new and first node
                expect_service = guessService(ns);
                participant->SetGuessService(expect_service);
            }
            new_node_lists_.push_back(name);
        }

        if (!expect_service.empty()){
            int32_t num_nodes = service_monitor_->NodeCount(expect_service);
            for (const auto& node: new_node_lists_){
                
                if (find(prev_node_lists_.begin(), prev_node_lists_.end(), node) != prev_node_lists_.end()){
                    // keep node - do nothing
                }else{
                    // new node 
                    num_nodes = service_monitor_->NodeStarted(expect_service, node);
                }
            }

            for (const auto& node: prev_node_lists_){
                
                if (find(new_node_lists_.begin(), new_node_lists_.end(), node) != new_node_lists_.end()){
                    // keep new node - do nothing 
                }else{
                    // detete node 
                    num_nodes = service_monitor_->NodeStopped(expect_service, node);
                }
            }
            LOG_DEBUG("[" << expect_service << "],  " << num_nodes << " running nodes " 
                            << ", " << prev_node_lists_.size() << ", " << new_node_lists_.size());
        }
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

std::string ROSServiceDiscovery::guessService(const std::string& ns) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < ns.size()) {
        size_t dashPos = ns.find('/', start);
        if (dashPos == std::string::npos) {
            tokens.push_back(ns.substr(start));
            break;
        }
        tokens.push_back(ns.substr(start, dashPos - start));
        start = dashPos + 1;
    }

    if (tokens.size() > 1){
        return tokens.at(1);
    }
    return "";
}

} // namespace adsw_manager
} // namespace ros2
