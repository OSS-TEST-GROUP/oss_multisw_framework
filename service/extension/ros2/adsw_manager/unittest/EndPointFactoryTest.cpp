// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/EndPointFactory.hpp"
#include "mock/MockDDSTransmitter.hpp"

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
        factory = std::make_shared<ros2::adsw_manager::EndPointFactory>();
        factory->Initialize();
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
    // You can add more checks for endpoints_ if you want
}

TEST_F(EndPointFactoryTest, CreateEndPoints) {
    auto transmitter = std::make_shared<MockDDSTransmitter>();
    size_t count = factory->CreateEndPoints(transmitter);
    EXPECT_EQ(count, factory->topic_lists_.size());
    // You can add more checks for endpoints_ if you want
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
