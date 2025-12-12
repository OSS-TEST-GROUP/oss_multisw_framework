// Unit tests for NotifyServiceStateEndPoint class using Google Test and Google Mock
// These tests are designed to achieve high code coverage including branch coverage
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/NotifyServiceStateEndPoint.hpp"

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
class NotifyServiceStateEndPointTest : public ::testing::Test {
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
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::NotifyServiceState_"));
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
    }
};

// ============================================================================
// Construction Tests - Using REAL NotifyServiceStateEndPoint class
// ============================================================================

TEST_F(NotifyServiceStateEndPointTest, ConstructWithNotifyServiceStateType) {
    // Create REAL endpoint instance
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(NotifyServiceStateEndPointTest, ConstructWithInvalidType) {
    // Create endpoint with a type that doesn't match - covers the else branch
    NotifyServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "invalid_topic");
    EXPECT_EQ(endpoint.GetTopic(), "invalid_topic");
}

TEST_F(NotifyServiceStateEndPointTest, GetTopic) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "my_topic");
    EXPECT_EQ(endpoint.GetTopic(), "my_topic");
}

TEST_F(NotifyServiceStateEndPointTest, GetTypeName) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(NotifyServiceStateEndPointTest, GetTypeSupport) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    EXPECT_NE(typeSupport, nullptr);
}

TEST_F(NotifyServiceStateEndPointTest, DestructorDoesNotCrash) {
    auto* endpoint = new NotifyServiceStateEndPoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

// ============================================================================
// OnDataAvailable Tests - Calling real implementation
// ============================================================================

TEST_F(NotifyServiceStateEndPointTest, OnDataAvailable_JustLogs) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    
    // OnDataAvailable just logs - verify it doesn't crash
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

TEST_F(NotifyServiceStateEndPointTest, OnDataAvailable_MultipleCalls) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    
    // Multiple calls should be fine
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

// ============================================================================
// Branch Coverage Tests
// ============================================================================

TEST_F(NotifyServiceStateEndPointTest, BranchCoverage_NotifyServiceState) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "notify_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(NotifyServiceStateEndPointTest, GetTopicReturnsCorrectValue) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "custom_topic");
    EXPECT_EQ(endpoint.GetTopic(), "custom_topic");
}

TEST_F(NotifyServiceStateEndPointTest, GetTopicWithEmptyString) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "");
    EXPECT_EQ(endpoint.GetTopic(), "");
}

TEST_F(NotifyServiceStateEndPointTest, GetTopicWithSpecialCharacters) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "/ros2/notify/state");
    EXPECT_EQ(endpoint.GetTopic(), "/ros2/notify/state");
}

TEST_F(NotifyServiceStateEndPointTest, GetTypeNameReturnsNonEmptyString) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
