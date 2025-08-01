// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/UpdateServiceListEndPoint.hpp"

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

// Mock UpdateServiceListResponse
class MockUpdateServiceListResponse : public UpdateServiceListResponse {
public:
    int32_t ret_code_ = 42;
    int32_t ret_code() const override { return ret_code_; }
};

class UpdateServiceListEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        endpoint = new ros2::adsw_manager::UpdateServiceListEndPoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "test_topic");
        endpoint->type_support_ = std::make_shared<MockTypeSupport>();
        endpoint->resp_ = MockUpdateServiceListResponse();
    }
    void TearDown() override {
        delete endpoint;
    }
    ros2::adsw_manager::UpdateServiceListEndPoint* endpoint;
};

TEST_F(UpdateServiceListEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(UpdateServiceListEndPointTest, OnDataAvailableDoesNotCrash) {
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}

TEST_F(UpdateServiceListEndPointTest, OnDataAvailableProcessesResponse) {
    endpoint->OnDataAvailable();
    EXPECT_EQ(endpoint->resp_.ret_code(), 42);
}
TEST_F(UpdateServiceListEndPointTest, GetTypeSupportReturnsValidType) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}
TEST_F(UpdateServiceListEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(UpdateServiceListEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
