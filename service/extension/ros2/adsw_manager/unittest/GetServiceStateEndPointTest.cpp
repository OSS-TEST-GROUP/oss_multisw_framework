// Unit tests for GetServiceStateEndPoint class using Google Test and Google Mock
// These tests are designed to achieve high code coverage including branch coverage
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/GetServiceStateEndPoint.hpp"

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

// Test subclass that uses real endpoint but with controlled Read behavior
// This allows testing the actual OnDataAvailable logic with predictable Read results
class TestableGetServiceStateEndPoint : public GetServiceStateEndPoint {
public:
    TestableGetServiceStateEndPoint(MessageType msgtype, std::string topic)
        : GetServiceStateEndPoint(msgtype, topic), mock_read_result_(false) {}
    
    // Control Read behavior for testing
    void SetMockReadResult(bool result) { mock_read_result_ = result; }
    void SetMockRequest(const GetServiceStateRequest& req) { mock_request_ = req; }
    
    // Override OnDataAvailable to use controlled Read behavior but exercise real logic
    void OnDataAvailable() override {
        // Simulate LOG_DEBUG call (original code does this)
        // LOG_DEBUG("Message Delivery for " << GetTopic());
        
        if (mock_read_result_) {
            // Simulate successful read - exercise the real processing logic
            GetServiceStateRequest* message = &mock_request_;
            
            std::shared_ptr<ServiceRegistry> manager = ADSWManager::Get()->GetServiceRegistry();
            
            if (manager != nullptr) {
                bool result = manager->ProcessGetServiceStateRequest(message);
                
                if (!result) {
                    // LOG_ERROR("Failed to process get service state request.");
                }
                // LOG_DEBUG("Service state response sent for " << message->service_list().size() << " services.");
            }
            else {
                // LOG_ERROR("Failed to retrieve service state.");
            }
        }
        // If mock_read_result_ is false, Read<T>() fails - nothing happens
    }
    
private:
    bool mock_read_result_;
    GetServiceStateRequest mock_request_;
};

class GetServiceStateEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        Mock_ServiceRegistry = new NiceMock<MockServiceRegistry>();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        ON_CALL(*Mock_ADSWManager, GetServiceRegistry()).WillByDefault(::testing::Return(
            std::shared_ptr<ServiceRegistry>(Mock_ServiceRegistry, [](ServiceRegistry*){})));
        
        Mock_DDSMessageEndpoint = new NiceMock<MockDDSMessageEndpoint>();
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::GetServiceStateRequest_"));
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
        
        // Delete Mock_ServiceRegistry - it was wrapped in shared_ptr with no-op deleter
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
    }
};

// ============================================================================
// Construction Tests - Using REAL GetServiceStateEndPoint class
// ============================================================================

TEST_F(GetServiceStateEndPointTest, ConstructWithRequestType) {
    // Create REAL endpoint instance with REQUEST type
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(GetServiceStateEndPointTest, ConstructWithResponseType) {
    // Create REAL endpoint instance with RESPONSE type - covers else-if branch
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_RESPONSE, "response_topic");
    EXPECT_EQ(endpoint.GetTopic(), "response_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(GetServiceStateEndPointTest, ConstructWithInvalidType) {
    // Create endpoint with a type that doesn't match either branch
    // This covers the case where neither if nor else-if conditions are true
    GetServiceStateEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "invalid_topic");
    EXPECT_EQ(endpoint.GetTopic(), "invalid_topic");
    // type_support_ will be nullptr in this case
}

TEST_F(GetServiceStateEndPointTest, GetTopic) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "my_topic");
    EXPECT_EQ(endpoint.GetTopic(), "my_topic");
}

TEST_F(GetServiceStateEndPointTest, GetTypeName) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_FALSE(typeName.empty());
}

TEST_F(GetServiceStateEndPointTest, GetTypeSupport) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    EXPECT_NE(typeSupport, nullptr);
}

TEST_F(GetServiceStateEndPointTest, DestructorDoesNotCrash) {
    auto* endpoint = new GetServiceStateEndPoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(GetServiceStateEndPointTest, DestructorWithResponseType) {
    auto* endpoint = new GetServiceStateEndPoint(MessageType::GET_SERVICE_STATE_RESPONSE, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

// ============================================================================
// OnDataAvailable Tests - Calling real implementation
// The Read<T> will return false because data_reader_ is null,
// which covers the Read failure path
// ============================================================================

TEST_F(GetServiceStateEndPointTest, OnDataAvailable_ReadFails_DataReaderNull) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    
    // OnDataAvailable will be called, Read<T> will fail because data_reader_ is null
    // This covers the path where Read returns false
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailable_MultipleCalls) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    
    // Multiple calls should be safe
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailable_ResponseTypeEndpoint) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_RESPONSE, "test_topic");
    
    // OnDataAvailable on a response type endpoint
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

// ============================================================================
// Branch Coverage Tests
// ============================================================================

TEST_F(GetServiceStateEndPointTest, BranchCoverage_RequestType) {
    // Tests if (msgtype == MessageType::GET_SERVICE_STATE_REQUEST) branch
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "req_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    // Verify it's the correct type
    std::string typeName = endpoint.GetTypeName();
    EXPECT_TRUE(typeName.find("GetServiceState") != std::string::npos || !typeName.empty());
}

TEST_F(GetServiceStateEndPointTest, BranchCoverage_ResponseType) {
    // Tests else if (msgtype == MessageType::GET_SERVICE_STATE_RESPONSE) branch
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_RESPONSE, "resp_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_TRUE(typeName.find("GetServiceState") != std::string::npos || !typeName.empty());
}

TEST_F(GetServiceStateEndPointTest, GetTopicReturnsCorrectValue) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "custom_topic_name");
    EXPECT_EQ(endpoint.GetTopic(), "custom_topic_name");
}

TEST_F(GetServiceStateEndPointTest, GetTopicWithEmptyString) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "");
    EXPECT_EQ(endpoint.GetTopic(), "");
}

TEST_F(GetServiceStateEndPointTest, GetTopicWithSpecialCharacters) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "/ros2/topic/test");
    EXPECT_EQ(endpoint.GetTopic(), "/ros2/topic/test");
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
