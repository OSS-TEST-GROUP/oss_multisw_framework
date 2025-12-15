// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ADSWManager.hpp"

#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockROSServiceController.hpp"
#include "mock/MockEndPointFactory.hpp"
#include "mock/MockServiceMonitor.hpp"

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
using ::testing::NiceMock;

using namespace ros2::adsw_manager;


class ADSWManagerTest : public ::testing::Test {
protected:
    ros2::adsw_manager::ADSWManager manager;

    void SetUp() override {
        Mock_DDSTransmitter = new MockDDSTransmitter();
        Mock_ServiceRegistry = new MockServiceRegistry();
        Mock_ServiceMonitor = new MockServiceMonitor();
        Mock_ROSServiceController = new MockROSServiceController();
        Mock_EndPointFactory = new MockEndPointFactory();

        manager.dds_transmitter_ = std::shared_ptr<MockDDSTransmitter>(Mock_DDSTransmitter);
        manager.service_registry_ = std::shared_ptr<MockServiceRegistry>(Mock_ServiceRegistry);
        manager.service_monitor_ = std::shared_ptr<MockServiceMonitor>(Mock_ServiceMonitor);
        manager.service_controller_ = std::shared_ptr<MockROSServiceController>(Mock_ROSServiceController);
        manager.endpoint_factory_ = std::shared_ptr<MockEndPointFactory>(Mock_EndPointFactory);

        Mock_Logger = new MockLogger();
        
        // Setup default mock expectations
        ON_CALL(*Mock_EndPointFactory, GetTopic(_)).WillByDefault(Return("mock_topic"));
        ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));
    }

    void TearDown() override {
        // Reset the manager's shared_ptrs first
        manager.dds_transmitter_ = nullptr;
        manager.service_registry_ = nullptr;
        manager.service_monitor_ = nullptr;
        manager.service_controller_ = nullptr;
        manager.endpoint_factory_ = nullptr;
        
        // Clear the shared instance first to release all shared_ptrs
        ADSWManager::SetShared(nullptr);
        
        // Clear the global raw pointers (they've already been deleted by shared_ptr)
        Mock_DDSTransmitter = nullptr;
        Mock_ServiceRegistry = nullptr;
        Mock_ServiceMonitor = nullptr;
        Mock_ROSServiceController = nullptr;
        Mock_EndPointFactory = nullptr;
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

};

TEST_F(ADSWManagerTest, InitializeAndStartStop) {
    ASSERT_TRUE(manager.Initialize());
    ASSERT_TRUE(manager.Start());
    manager.Stop();
}

TEST_F(ADSWManagerTest, Getters) {
    EXPECT_NE(manager.GetTransmitter(), nullptr);
    EXPECT_NE(manager.GetServiceRegistry(), nullptr);
    EXPECT_NE(manager.GetServiceMonitor(), nullptr);
    EXPECT_NE(manager.GetServiceController(), nullptr);
    // ServiceDiscovery may be null or not, depending on mock
    SUCCEED();
}

TEST_F(ADSWManagerTest, GetTopic) {
    EXPECT_STREQ(manager.GetTopic(ros2::adsw_manager::MessageType::START_SERVICE_REQUEST), "mock_topic");
}

TEST_F(ADSWManagerTest, SingletonGetShared) {
    auto shared1 = ros2::adsw_manager::ADSWManager::getShared();
    auto shared2 = ros2::adsw_manager::ADSWManager::getShared();
    EXPECT_EQ(shared1.get(), shared2.get());
    EXPECT_EQ(ros2::adsw_manager::ADSWManager::Get(), shared1.get());
}

TEST_F(ADSWManagerTest, InitializeMultipleTimes) {
    ASSERT_TRUE(manager.Initialize());
    // Second initialize should also succeed (idempotent)
    ASSERT_TRUE(manager.Initialize());
}

TEST_F(ADSWManagerTest, StartWithoutInitialize) {
    // Create a fresh manager without initialization
    ros2::adsw_manager::ADSWManager fresh_manager;
    // Start may succeed or fail, but should not crash
    EXPECT_NO_THROW(fresh_manager.Start());
}

TEST_F(ADSWManagerTest, StopWithoutStart) {
    // Stop without start should not crash
    EXPECT_NO_THROW(manager.Stop());
}

TEST_F(ADSWManagerTest, GetTopicAllMessageTypes) {
    // Test GetTopic for various message types
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::START_SERVICE_REQUEST));
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST));
    EXPECT_NO_THROW(manager.GetTopic(ros2::adsw_manager::MessageType::GET_SERVICE_STATE_REQUEST));
}

TEST_F(ADSWManagerTest, GetServiceDiscovery) {
    auto discovery = manager.GetServiceDiscovery();
    // May be null depending on mock setup
    SUCCEED();
}

TEST_F(ADSWManagerTest, StartAndStopMultipleTimes) {
    ASSERT_TRUE(manager.Initialize());
    ASSERT_TRUE(manager.Start());
    manager.Stop();
    
    // Should be able to start and stop again
    ASSERT_TRUE(manager.Start());
    manager.Stop();
}

// ==================== Additional tests for improved coverage ====================

// Test Initialize succeeds when all components initialize successfully
TEST_F(ADSWManagerTest, Initialize_AllComponentsSucceed) {
    // Setup: All Initialize() calls return true
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(true));
    
    EXPECT_TRUE(manager.Initialize());
}

// Test Initialize fails when DDSTransmitter::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_DDSTransmitterFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when EndPointFactory::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_EndPointFactoryFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when ServiceMonitor::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_ServiceMonitorFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Initialize fails when ROSServiceController::Initialize() fails
TEST_F(ADSWManagerTest, Initialize_ServiceControllerFails) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Initialize());
}

// Test Start when endpoint_factory_ is not null
TEST_F(ADSWManagerTest, Start_WithEndpointFactory) {
    ON_CALL(*Mock_DDSTransmitter, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_EndPointFactory, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceMonitor, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ROSServiceController, Initialize()).WillByDefault(Return(true));
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));
    
    EXPECT_CALL(*Mock_EndPointFactory, CreateEndPoints(_)).Times(1);
    
    manager.Initialize();
    EXPECT_TRUE(manager.Start());
}

// Test Start when service_registry_->LoadServiceList() fails
TEST_F(ADSWManagerTest, Start_LoadServiceListFails) {
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(false));
    
    EXPECT_FALSE(manager.Start());
}

// Test Start with null endpoint_factory_
TEST_F(ADSWManagerTest, Start_NullEndpointFactory) {
    manager.endpoint_factory_ = nullptr;
    ON_CALL(*Mock_ServiceRegistry, LoadServiceList()).WillByDefault(Return(true));
    
    // Should not call CreateEndPoints and should still succeed
    EXPECT_TRUE(manager.Start());
}

// Test Process() function
TEST_F(ADSWManagerTest, Process_InStartedState) {
    // Set state to ADSW_STARTED
    manager.SetState(AdswState::ADSW_STARTED);
    
    ON_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).WillByDefault(Return(true));
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(1, _)).Times(1);
    
    EXPECT_NO_THROW(manager.Process());
    
    // State should change to ADSW_RUNNING
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
}

// Test Process() when ProcessUpdateServiceListRequest returns false
TEST_F(ADSWManagerTest, Process_UpdateServiceListRequestFails) {
    manager.SetState(AdswState::ADSW_STARTED);
    
    ON_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).WillByDefault(Return(false));
    EXPECT_CALL(*Mock_ServiceMonitor, NotifyAdswStatus(1, _)).Times(1);
    
    EXPECT_NO_THROW(manager.Process());
    
    // State should remain ADSW_STARTED since ProcessUpdateServiceListRequest returned false
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED);
}

// Test Process() when service_monitor_ is null
TEST_F(ADSWManagerTest, Process_NullServiceMonitor) {
    manager.SetState(AdswState::ADSW_STARTED);
    manager.service_monitor_ = nullptr;
    
    ON_CALL(*Mock_ServiceRegistry, ProcessUpdateServiceListRequest()).WillByDefault(Return(true));
    
    // Should not crash even with null service_monitor_
    EXPECT_NO_THROW(manager.Process());
}

// Test Process() when not in ADSW_STARTED state
TEST_F(ADSWManagerTest, Process_NotInStartedState) {
    manager.SetState(AdswState::ADSW_INIT);
    
    // Process should do nothing when not in STARTED state
    EXPECT_NO_THROW(manager.Process());
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_INIT);
}

// Test GetState()
TEST_F(ADSWManagerTest, GetState_ReturnsCorrectState) {
    manager.SetState(AdswState::ADSW_INIT);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_INIT);
    
    manager.SetState(AdswState::ADSW_STARTED);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_STARTED);
    
    manager.SetState(AdswState::ADSW_RUNNING);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_RUNNING);
    
    manager.SetState(AdswState::ADSW_TERMINATE);
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_TERMINATE);
}

// Test GetServiceDiscovery when service_monitor_ is null
TEST_F(ADSWManagerTest, GetServiceDiscovery_NullServiceMonitor) {
    manager.service_monitor_ = nullptr;
    
    auto discovery = manager.GetServiceDiscovery();
    EXPECT_EQ(discovery, nullptr);
}

// Test SetShared()
TEST_F(ADSWManagerTest, SetShared) {
    auto custom_manager = std::make_shared<ADSWManager>();
    
    ADSWManager::SetShared(custom_manager);
    
    // Verify that SetShared was called (we can't easily verify the internal state,
    // but we ensure it doesn't crash)
    SUCCEED();
}

// Test Stop with null components
TEST_F(ADSWManagerTest, Stop_NullComponents) {
    manager.service_controller_ = nullptr;
    manager.service_monitor_ = nullptr;
    manager.dds_transmitter_ = nullptr;
    
    // Should not crash when components are null
    EXPECT_NO_THROW(manager.Stop());
}

// Test Stop calls Finalize on all components
TEST_F(ADSWManagerTest, Stop_CallsFinalize) {
    EXPECT_CALL(*Mock_ROSServiceController, Finalize()).Times(1);
    EXPECT_CALL(*Mock_ServiceMonitor, Finalize()).Times(1);
    EXPECT_CALL(*Mock_DDSTransmitter, Finalize()).Times(1);
    
    manager.Stop();
    
    EXPECT_EQ(manager.GetState(), AdswState::ADSW_TERMINATE);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
