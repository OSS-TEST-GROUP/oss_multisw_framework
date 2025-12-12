// Unit tests for UpdateServiceListEndPoint class using Google Test and Google Mock
// These tests are designed to achieve high code coverage including branch coverage
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/UpdateServiceListEndPoint.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockTypeSupport.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
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

// Direct test class - creates real instances to test actual implementation
class UpdateServiceListEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        
        Mock_DDSMessageEndpoint = new NiceMock<MockDDSMessageEndpoint>();
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::UpdateServiceListRequest_"));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeSupport()).WillByDefault(Return(nullptr));
    }
    
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;
        Mock_ADSWManager = nullptr;
    }
};

// ============================================================================
// Construction Tests - Using REAL UpdateServiceListEndPoint class
// ============================================================================

TEST_F(UpdateServiceListEndPointTest, ConstructWithRequestType) {
    // Create REAL endpoint instance with REQUEST type
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(UpdateServiceListEndPointTest, ConstructWithResponseType) {
    // Create REAL endpoint instance with RESPONSE type - covers else-if branch
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "response_topic");
    EXPECT_EQ(endpoint.GetTopic(), "response_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(UpdateServiceListEndPointTest, ConstructWithInvalidType) {
    // Create endpoint with a type that doesn't match either branch
    UpdateServiceListEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "invalid_topic");
    EXPECT_EQ(endpoint.GetTopic(), "invalid_topic");
}

TEST_F(UpdateServiceListEndPointTest, GetTopic) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "my_topic");
    EXPECT_EQ(endpoint.GetTopic(), "my_topic");
}

TEST_F(UpdateServiceListEndPointTest, GetTypeName) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(UpdateServiceListEndPointTest, GetTypeSupport) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    EXPECT_NE(typeSupport, nullptr);
}

TEST_F(UpdateServiceListEndPointTest, DestructorDoesNotCrash) {
    auto* endpoint = new UpdateServiceListEndPoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(UpdateServiceListEndPointTest, DestructorWithResponseType) {
    auto* endpoint = new UpdateServiceListEndPoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

// ============================================================================
// Branch Coverage Tests
// ============================================================================

TEST_F(UpdateServiceListEndPointTest, BranchCoverage_RequestType) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "req_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(UpdateServiceListEndPointTest, BranchCoverage_ResponseType) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "resp_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(UpdateServiceListEndPointTest, GetTopicReturnsCorrectValue) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "custom_topic_name");
    EXPECT_EQ(endpoint.GetTopic(), "custom_topic_name");
}

TEST_F(UpdateServiceListEndPointTest, GetTopicWithEmptyString) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "");
    EXPECT_EQ(endpoint.GetTopic(), "");
}

TEST_F(UpdateServiceListEndPointTest, GetTopicWithSpecialCharacters) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "/ros2/update/service");
    EXPECT_EQ(endpoint.GetTopic(), "/ros2/update/service");
}

TEST_F(UpdateServiceListEndPointTest, GetTypeNameReturnsNonEmptyString) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
