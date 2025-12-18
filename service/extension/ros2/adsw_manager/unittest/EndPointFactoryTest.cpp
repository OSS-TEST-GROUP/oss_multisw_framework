// Unit tests for EndPointFactory class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/EndPointFactory.hpp"

#include "mock/MockDDSTransmitter.hpp"
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


class EndPointFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
        factory = std::make_shared<ros2::adsw_manager::EndPointFactory>();
        factory->Initialize();
    }
    
    void TearDown() override {
        factory.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
    
    std::shared_ptr<ros2::adsw_manager::EndPointFactory> factory;
};

TEST_F(EndPointFactoryTest, InitializeAndFinalize) {
    ASSERT_TRUE(factory->Initialize());
    factory->Finalize();
}

TEST_F(EndPointFactoryTest, GetTopic) {
    EXPECT_STREQ(factory->GetTopic(MessageType::ROS2_DISCOVREY_INFO), "ros_discovery_info");
    EXPECT_STREQ(factory->GetTopic(MessageType::START_SERVICE_REQUEST), "mmf/lifecyclemgr/StartServiceRequest");
    EXPECT_EQ(factory->GetTopic(static_cast<MessageType>(999)), nullptr); // Unknown type
}

TEST_F(EndPointFactoryTest, CreateEndPoints) {
    auto transmitter = std::make_shared<MockDDSTransmitter>();
    size_t count = factory->CreateEndPoints(transmitter);
    EXPECT_EQ(count, factory->topic_lists_.size());
}

TEST_F(EndPointFactoryTest, GetTopicAllTypes) {
    // Test GetTopic for all known message types
    EXPECT_NE(factory->GetTopic(MessageType::ROS2_DISCOVREY_INFO), nullptr);
    EXPECT_NE(factory->GetTopic(MessageType::START_SERVICE_REQUEST), nullptr);
    EXPECT_NE(factory->GetTopic(MessageType::STOP_SERVICE_REQUEST), nullptr);
    EXPECT_NE(factory->GetTopic(MessageType::GET_SERVICE_STATE_REQUEST), nullptr);
}

TEST_F(EndPointFactoryTest, GetTopicInvalidType) {
    EXPECT_EQ(factory->GetTopic(static_cast<MessageType>(999)), nullptr);
    EXPECT_EQ(factory->GetTopic(static_cast<MessageType>(-1)), nullptr);
}

TEST_F(EndPointFactoryTest, InitializeMultipleTimes) {
    EXPECT_TRUE(factory->Initialize());
    EXPECT_TRUE(factory->Initialize());  // Should be idempotent
}

TEST_F(EndPointFactoryTest, FinalizeMultipleTimes) {
    factory->Initialize();
    factory->Finalize();
    EXPECT_NO_THROW(factory->Finalize());  // Should not crash
}

TEST_F(EndPointFactoryTest, CreateEndPointsWithNullTransmitter) {
    size_t count = factory->CreateEndPoints(nullptr);
    EXPECT_EQ(count, 0);
}

TEST_F(EndPointFactoryTest, FinalizeWithoutInitialize) {
    ros2::adsw_manager::EndPointFactory fresh_factory;
    EXPECT_NO_THROW(fresh_factory.Finalize());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
