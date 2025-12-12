// Unit tests for StartServiceEndPoint class using Google Test and Google Mock
// These tests are designed to achieve high code coverage including branch coverage
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/StartServiceEndPoint.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockTypeSupport.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#include "mock/MockROSServiceController.hpp"
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
class StartServiceEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        Mock_ROSServiceController = new NiceMock<MockROSServiceController>();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        ON_CALL(*Mock_ADSWManager, GetServiceController()).WillByDefault(::testing::Return(
            std::shared_ptr<ROSServiceController>(Mock_ROSServiceController, [](ROSServiceController*){})));
        
        Mock_DDSMessageEndpoint = new NiceMock<MockDDSMessageEndpoint>();
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::StartServiceRequest_"));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeSupport()).WillByDefault(Return(nullptr));
    }
    
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;
        
        // Reset the static shared_ptr - this will delete Mock_ADSWManager
        ADSWManager::SetShared(nullptr);
        Mock_ADSWManager = nullptr;
        
        // Delete Mock_ROSServiceController - it was wrapped in shared_ptr with no-op deleter
        delete Mock_ROSServiceController;
        Mock_ROSServiceController = nullptr;
    }
};

// ============================================================================
// Construction Tests - Using REAL StartServiceEndPoint class
// ============================================================================

TEST_F(StartServiceEndPointTest, ConstructWithRequestType) {
    // Create REAL endpoint instance with REQUEST type
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(StartServiceEndPointTest, ConstructWithResponseType) {
    // Create REAL endpoint instance with RESPONSE type - covers else-if branch
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_RESPONSE, "response_topic");
    EXPECT_EQ(endpoint.GetTopic(), "response_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(StartServiceEndPointTest, ConstructWithInvalidType) {
    // Create endpoint with a type that doesn't match either branch
    StartServiceEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "invalid_topic");
    EXPECT_EQ(endpoint.GetTopic(), "invalid_topic");
}

TEST_F(StartServiceEndPointTest, GetTopic) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "my_topic");
    EXPECT_EQ(endpoint.GetTopic(), "my_topic");
}

TEST_F(StartServiceEndPointTest, GetTypeName) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(StartServiceEndPointTest, GetTypeSupport) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    EXPECT_NE(typeSupport, nullptr);
}

TEST_F(StartServiceEndPointTest, DestructorDoesNotCrash) {
    auto* endpoint = new StartServiceEndPoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(StartServiceEndPointTest, DestructorWithResponseType) {
    auto* endpoint = new StartServiceEndPoint(MessageType::START_SERVICE_RESPONSE, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

// ============================================================================
// Branch Coverage Tests
// ============================================================================

TEST_F(StartServiceEndPointTest, BranchCoverage_RequestType) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "req_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(StartServiceEndPointTest, BranchCoverage_ResponseType) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_RESPONSE, "resp_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(StartServiceEndPointTest, GetTopicReturnsCorrectValue) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "custom_topic_name");
    EXPECT_EQ(endpoint.GetTopic(), "custom_topic_name");
}

TEST_F(StartServiceEndPointTest, GetTopicWithEmptyString) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "");
    EXPECT_EQ(endpoint.GetTopic(), "");
}

TEST_F(StartServiceEndPointTest, GetTopicWithSpecialCharacters) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "/ros2/start/service");
    EXPECT_EQ(endpoint.GetTopic(), "/ros2/start/service");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
