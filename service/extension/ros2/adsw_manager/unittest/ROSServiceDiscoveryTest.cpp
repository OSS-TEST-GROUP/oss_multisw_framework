// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ROSServiceController.hpp"
#include "monitor/UpdateServiceInfo.hpp"

#include "mock/MockServiceMonitor.hpp"
#include "mock/MockNodeInfo.hpp"
#include "mock/MockContainerController.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

using namespace ros2::adsw_manager;

class ROSServiceDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor = new MockServiceMonitor();
        discovery = new ROSServiceDiscovery(monitor);
    }
    void TearDown() override {
        delete discovery;
        delete monitor;
    }
    ROSServiceDiscovery* discovery;
    MockServiceMonitor* monitor;
};

TEST_F(ROSServiceDiscoveryTest, InitializeAndFinalize) {
    ASSERT_TRUE(discovery->Initialize());
    discovery->Finalize();
}

TEST_F(ROSServiceDiscoveryTest, GuessService) {
    EXPECT_EQ(discovery->guessService("/foo/bar"), "foo");
    EXPECT_EQ(discovery->guessService("/foo"), "");
    EXPECT_EQ(discovery->guessService(""), "");
}

TEST_F(ROSServiceDiscoveryTest, ParseROSDiscoveryInfo_NewParticipant) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    // You would need to adapt this to your actual rmw_dds_common::msg::ParticipantEntitiesInfo type
    // For real tests, use the actual ROS 2 message type and fill fields accordingly
    // discovery->ParseROSDiscoveryInfo(info);
    // EXPECT_TRUE(discovery->GetROSParticipant("test_gid") != nullptr);
}

TEST_F(ROSServiceDiscoveryTest, ParseROSDiscoveryInfo_EmptyNodes) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    // info.node_entities_info_seq_ is empty
    // discovery->ParseROSDiscoveryInfo(info);
    // Should not create a participant
    // EXPECT_TRUE(discovery->GetROSParticipant("test_gid") == nullptr);
}

TEST_F(ROSServiceDiscoveryTest, ParseROSDiscoveryInfo_UpdateNodeList) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    // discovery->ParseROSDiscoveryInfo(info);
    
    // Simulate a new node
    info.node_entities_info_seq_.push_back({"ns", "node2"});
    // discovery->ParseROSDiscoveryInfo(info);
    
    // Check if the new node was added
    auto participant = discovery->GetROSParticipant("test_gid");
    EXPECT_TRUE(participant != nullptr);
    EXPECT_EQ(participant->nodes.size(), 2);
}
TEST_F(ROSServiceDiscoveryTest, ParseROSDiscoveryInfo_NodeCount) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    // discovery->ParseROSDiscoveryInfo(info);
    
    // Simulate a node started
    monitor->NodeStarted("test_service", "node1");
    EXPECT_EQ(monitor->NodeCount("test_service"), 1);
    
    // Simulate a node stopped
    monitor->NodeStopped("test_service", "node1");
    EXPECT_EQ(monitor->NodeCount("test_service"), 0);
}
TEST_F(ROSServiceDiscoveryTest, GetROSParticipant_NullParticipant) {
    auto participant = discovery->GetROSParticipant("non_existent_gid");
    EXPECT_TRUE(participant == nullptr);
}
TEST_F(ROSServiceDiscoveryTest, GetROSParticipant_ValidParticipant) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    // discovery->ParseROSDiscoveryInfo(info);
    
    auto participant = discovery->GetROSParticipant("test_gid");
    EXPECT_TRUE(participant != nullptr);
    EXPECT_EQ(participant->nodes.size(), 1);
}
TEST_F(ROSServiceDiscoveryTest, GetROSParticipant_EmptyParticipant) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    // No nodes added
    // discovery->ParseROSDiscoveryInfo(info);
    
    auto participant = discovery->GetROSParticipant("test_gid");
    EXPECT_TRUE(participant != nullptr);
    EXPECT_EQ(participant->nodes.size(), 0);
}

TEST_F(ROSServiceDiscoveryTest, GetROSParticipant) {
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    // discovery->ParseROSDiscoveryInfo(info);
    auto participant = discovery->GetROSParticipant("test_gid");
    EXPECT_TRUE(participant != nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
