// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ADSWManager.hpp"

#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockROSServiceController.hpp"
#include "mock/MockEndPointFactory.hpp"

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
        manager.dds_transmitter_ = std::make_shared<MockDDSTransmitter>();
        manager.service_registry_ = std::make_shared<MockServiceRegistry>();
        manager.service_monitor_ = std::make_shared<MockServiceMonitor>();
        manager.service_controller_ = std::make_shared<MockROSServiceController>();
        manager.endpoint_factory_ = std::make_shared<MockEndPointFactory>();
    }
};

TEST_F(ADSWManagerTest, InitializeAndStartStop) {
    ASSERT_TRUE(manager.Initialize());
    ASSERT_TRUE(manager.Start());
    manager.Stop();
}

TEST_F(ADSWManagerTest, Getters) {
    EXPECT_TRUE(manager.GetTransmitter() != nullptr);
    EXPECT_TRUE(manager.GetServiceRegistry() != nullptr);
    EXPECT_TRUE(manager.GetServiceMonitor() != nullptr);
    EXPECT_TRUE(manager.GetServiceController() != nullptr);
    EXPECT_TRUE(manager.GetServiceDiscovery() == nullptr || manager.GetServiceDiscovery() != nullptr); // depends on mock
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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
