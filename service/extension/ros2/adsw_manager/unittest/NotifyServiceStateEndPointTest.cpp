// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/NotifyServiceStateEndPoint.hpp"


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

class NotifyServiceStateEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        endpoint = new ros2::adsw_manager::NotifyServiceStateEndPoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
        endpoint->type_support_ = std::make_shared<MockTypeSupport>();
    }
    void TearDown() override {
        delete endpoint;
    }
    ros2::adsw_manager::NotifyServiceStateEndPoint* endpoint;
};

TEST_F(NotifyServiceStateEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(NotifyServiceStateEndPointTest, OnDataAvailableDoesNotCrash) {
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}
TEST_F(NotifyServiceStateEndPointTest, OnDataAvailableDoesNotProcess) {
    // Since NotifyServiceState does not process data, we just check that it doesn't crash
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}
TEST_F(NotifyServiceStateEndPointTest, GetTypeSupportReturnsValidType) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}
TEST_F(NotifyServiceStateEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(NotifyServiceStateEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
}
TEST_F(NotifyServiceStateEndPointTest, TypeSupportIsNotNull) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
