
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_NODE_INFO_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_NODE_INFO_HPP_

#include <gmock/gmock.h>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

// Mock ROS2Node and ROS2Participant


class MockNodeEntitiesInfo : public rmw_dds_common::msg::NodeEntitiesInfo {
public:
    MockNodeEntitiesInfo() : rmw_dds_common::msg::NodeEntitiesInfo() {}
    MockNodeEntitiesInfo(const std::string& node_namespace, const std::string& node_name) {
        this->node_namespace = node_namespace;
        this->node_name = node_name;
    }
    std::string node_namespace;
    std::string node_name;
};

class MockParticipantEntitiesInfo  : public rmw_dds_common::msg::ParticipantEntitiesInfo {
public:
    std::vector<MockNodeEntitiesInfo> node_entities_info_seq_;
    std::string participant_gid_;
    std::vector<MockNodeEntitiesInfo>& node_entities_info_seq() { return node_entities_info_seq_; }
    std::string participant_gid() { return participant_gid_; }
};



}
}

#endif