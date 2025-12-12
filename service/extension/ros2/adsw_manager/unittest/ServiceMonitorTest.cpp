// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "monitor/ServiceMonitor.cpp"  // Include implementation directly

#include "mock/MockADSWManager.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockService.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockDDS.hpp"
#include "mock/MockNodeInfo.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#define MOCK_ROS_SERVICE_DISCOVERY_IMPL
#include "mock/MockROSServiceDiscovery.hpp"
#include "mock/MockUpdateServiceInfo.hpp"
#include "mock/MockLogger.hpp"

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


// Test fixture
class ServiceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        registry = new MockServiceRegistry();
        monitor = new ServiceMonitor();
        monitor->Initialize();
    }
    void TearDown() override {
        monitor->Finalize();
        delete monitor;
        delete registry;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
    ServiceMonitor* monitor;
    MockServiceRegistry* registry = nullptr;
};


TEST_F(ServiceMonitorTest, InitializeAndFinalize) {
    ASSERT_TRUE(monitor->Initialize());
    monitor->Finalize();
}

TEST_F(ServiceMonitorTest, NodeStartedAndNodeCount) {
    std::string service_name = "test_service";
    EXPECT_EQ(monitor->NodeCount(service_name), 0);
    monitor->NodeStarted(service_name, "node1");
    EXPECT_EQ(monitor->NodeCount(service_name), 1);
    monitor->NodeStarted(service_name, "node2");
    EXPECT_EQ(monitor->NodeCount(service_name), 2);
}

TEST_F(ServiceMonitorTest, NodeStopped) {
    std::string service_name = "test_service";
    monitor->NodeStarted(service_name, "node1");
    monitor->NodeStarted(service_name, "node2");
    EXPECT_EQ(monitor->NodeCount(service_name), 2);
    monitor->NodeStopped(service_name, "node1");
    EXPECT_EQ(monitor->NodeCount(service_name), 1);
    monitor->NodeStopped(service_name, "node2");
    EXPECT_EQ(monitor->NodeCount(service_name), 0);
}

TEST_F(ServiceMonitorTest, UpdateServiceState) {
    auto service = std::make_shared<MockService>("test_service");
    registry->AddService(service, "test_group");
    monitor->NodeStarted("test_service", "node1");
    monitor->UpdateServiceState();
    EXPECT_EQ(service->GetRunningNodes(), 1);
    monitor->NodeStopped("test_service", "node1");
    monitor->UpdateServiceState();
    EXPECT_EQ(service->GetRunningNodes(), 0);
}
TEST_F(ServiceMonitorTest, UpdateServiceStateWithEvent) {
    UpdateServiceInfo info("test_service", "node1", ServiceEvent::CREATED, ros2::adsw_manager::ServiceState::INITIALIZING);
    monitor->UpdateServiceState(info);
    auto service = registry->GetService("test_service");
    ASSERT_TRUE(service != nullptr);
    EXPECT_EQ(service->GetServiceState(), ros2::adsw_manager::ServiceState::INITIALIZING);
    
    info.service_event = ServiceEvent::STATE_UPDATED;
    info.state = ros2::adsw_manager::ServiceState::RUNNING;
    monitor->UpdateServiceState(info);
    EXPECT_EQ(service->GetServiceState(), ros2::adsw_manager::ServiceState::RUNNING);
}

TEST_F(ServiceMonitorTest, NotifyServiceState) {
    auto service = std::make_shared<MockService>("test_service");
    registry->AddService(service, "test_group");
    monitor->NodeStarted("test_service", "node1");
    
    // Mock the endpoint to verify the notification
    auto mock_endpoint = std::make_shared<MockDDSMessageEndpoint>();
    EXPECT_CALL(*mock_endpoint, Write(_)).Times(1);
    
    monitor->NotifyServiceState();
    // Check if the notification was sent
    EXPECT_EQ(service->GetRunningNodes(), 1);
}
TEST_F(ServiceMonitorTest, GetServiceDiscovery) {
    auto discovery = monitor->GetServiceDiscovery();
    ASSERT_TRUE(discovery != nullptr);
    EXPECT_EQ(discovery->GetROSParticipant("test_gid"), nullptr);  // No participant added yet
}
TEST_F(ServiceMonitorTest, GetServiceDiscoveryWithParticipant) {
    auto discovery = monitor->GetServiceDiscovery();
    ASSERT_TRUE(discovery != nullptr);
    
    // Simulate adding a participant
    MockParticipantEntitiesInfo info;
    info.participant_gid_ = "test_gid";
    info.node_entities_info_seq_.push_back({"ns", "node1"});
    
    discovery->ParseROSDiscoveryInfo(info);
    
    auto participant = discovery->GetROSParticipant("test_gid");
    ASSERT_TRUE(participant != nullptr);
    EXPECT_EQ(participant->nodes.size(), 1);
    EXPECT_EQ(participant->nodes[0].node_name, "node1");
}

TEST_F(ServiceMonitorTest, NodeStartedMultipleServices) {
    monitor->NodeStarted("service1", "node1");
    monitor->NodeStarted("service2", "node2");
    
    EXPECT_EQ(monitor->NodeCount("service1"), 1);
    EXPECT_EQ(monitor->NodeCount("service2"), 1);
}

TEST_F(ServiceMonitorTest, NodeStoppedNonExistent) {
    // Stop a node that was never started - should not crash
    EXPECT_NO_THROW(monitor->NodeStopped("nonexistent", "node1"));
    EXPECT_EQ(monitor->NodeCount("nonexistent"), 0);
}

TEST_F(ServiceMonitorTest, NodeCountEmptyService) {
    EXPECT_EQ(monitor->NodeCount(""), 0);
}

TEST_F(ServiceMonitorTest, InitializeMultipleTimes) {
    EXPECT_TRUE(monitor->Initialize());
    EXPECT_TRUE(monitor->Initialize());  // Should be idempotent
}

TEST_F(ServiceMonitorTest, FinalizeMultipleTimes) {
    monitor->Finalize();
    EXPECT_NO_THROW(monitor->Finalize());  // Should not crash
}

TEST_F(ServiceMonitorTest, NodeStartedDuplicateNode) {
    monitor->NodeStarted("service1", "node1");
    monitor->NodeStarted("service1", "node1");  // Duplicate
    
    // Behavior depends on implementation - either 1 or 2
    EXPECT_GE(monitor->NodeCount("service1"), 1);
}

TEST_F(ServiceMonitorTest, NodeStoppedMoreThanStarted) {
    monitor->NodeStarted("service1", "node1");
    monitor->NodeStopped("service1", "node1");
    monitor->NodeStopped("service1", "node1");  // Extra stop
    
    EXPECT_EQ(monitor->NodeCount("service1"), 0);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
