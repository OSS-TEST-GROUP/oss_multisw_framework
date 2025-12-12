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
        manager.dds_transmitter_ = nullptr;
        manager.service_registry_ = nullptr;
        manager.service_monitor_ = nullptr;
        manager.service_controller_ = nullptr;
        manager.endpoint_factory_ = nullptr;
        
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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
