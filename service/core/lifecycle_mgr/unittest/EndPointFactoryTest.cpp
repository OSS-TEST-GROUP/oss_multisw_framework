// Unit tests for EndPointFactory class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/EndPointFactory.hpp"
#include "endpoint/StartServiceEndPoint.hpp"
#include "endpoint/StopServiceEndPoint.hpp"
#include "endpoint/GetServiceStateEndPoint.hpp"
#include "endpoint/NotifyServiceStateEndPoint.hpp"
#include "endpoint/SetServiceModeEndPoint.hpp"
#include "endpoint/UpdateServiceListEndPoint.hpp"
#include "endpoint/LifecycleDiagReportEndPoint.hpp"

#include "mock/mock_dds_message_endpoint.hpp"
#include "mock/mock_dds_transmitter.hpp"
#include "mock/mock_message_endpoint.hpp"
#include "mock/mock_lifecycle_factory.hpp"
#include "mock/mock_state_monitor.hpp"
#include "mock/mock_state_manager.hpp"
#include "mock/mock_manager_base.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_logger.hpp"

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
using ::testing::Invoke;

using namespace lifecycle_manager;


LifecycleFactory* LifecycleFactory::GetInstance() { 
    static LifecycleFactory inst; 
    return &inst; 
}

class EndPointFactoryTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        Mock_Logger = new MockLogger();
        Mock_DDSTransmitter = new MockDDSTransmitter();
        MockMessageEndPoint::Mock_MessageEndPoint = new MockMessageEndPoint("test_topic");
        Mock_DDSMessageEndPoint = new MockDDSMessageEndPoint("test_topic");
    }
    virtual void TearDown(){
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete Mock_DDSTransmitter;
        Mock_DDSTransmitter = nullptr;
        delete MockMessageEndPoint::Mock_MessageEndPoint;
        MockMessageEndPoint::Mock_MessageEndPoint = nullptr;
        delete Mock_DDSMessageEndPoint;
        Mock_DDSMessageEndPoint = nullptr;
    }
};

TEST_F(EndPointFactoryTest, CreateEndPointsCalled) {
    EndPointFactory factory;
    auto dds = std::make_shared<DDSTransmitter>();

    
    factory.CreateEndPoints(dds);
}

// Add more tests for EndPointFactory as needed
TEST_F(EndPointFactoryTest, InitializeReturnsCorrectSize) {
    EndPointFactory factory;
    int32_t size = factory.Initialize();
    // There are 12 message types initialized in the factory
    EXPECT_EQ(size, 12);
}

TEST_F(EndPointFactoryTest, GetTopicReturnsCorrectTopic) {
    EndPointFactory factory;
    factory.Initialize();
    EXPECT_STREQ(factory.GetTopic(MessageType::LIFECYCLE_DIAG_REPORT), "mmf/lifecyclemgr/LifecycleDiagReport");
    EXPECT_STREQ(factory.GetTopic(MessageType::SET_SERVICE_MODE_REQUST), "mmf/lifecyclemgr/SetServiceModeRequest");
    EXPECT_STREQ(factory.GetTopic(MessageType::UPDATE_SERVICE_LIST_RESPONSE), "mmf/lifecyclemgr/UpdateServiceListResponse");
}

TEST_F(EndPointFactoryTest, GetTopicReturnsNullptrForInvalidType) {
    EndPointFactory factory;
    factory.Initialize();
    // Assuming MessageType(-1) is invalid
    EXPECT_EQ(factory.GetTopic(static_cast<MessageType>(-1)), nullptr);
}

TEST_F(EndPointFactoryTest, CreateEndPointsReturnsCorrectSize) {
    EndPointFactory factory;

    factory.SetCustomCreator(MessageType::UPDATE_SERVICE_LIST_RESPONSE,
        [](MessageType type, const std::string& topic) {
            auto mock = new MockDDSMessageEndPoint(topic);
            Mock_DDSMessageEndPoint = mock; // set the global pointer
            return std::shared_ptr<MockDDSMessageEndPoint>(mock);
    }   );

    factory.Initialize();

    // Set up expectations for GetDomain, GetPublisher, GetSubscriber if needed
    EXPECT_CALL(*Mock_DDSTransmitter, GetDomain()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*Mock_DDSTransmitter, GetPublisher()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*Mock_DDSTransmitter, GetSubscriber()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).Times(12);

    //int32_t size = factory.CreateEndPoints(dds);
    //EXPECT_EQ(size, 12);
}

// Additional tests for branch coverage

// Test GetTopic for all message types
TEST_F(EndPointFactoryTest, GetTopic_AllMessageTypes) {
    EndPointFactory factory;
    factory.Initialize();
    
    EXPECT_NE(factory.GetTopic(MessageType::LIFECYCLE_DIAG_REPORT), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::GET_SERVICE_STATE_REQUEST), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::GET_SERVICE_STATE_RESPONSE), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::NOTIFY_SERVICE_STATE), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::SET_SERVICE_MODE_REQUST), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::SET_SERVICE_MODE_RESPONSE), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::START_SERVICE_REQUEST), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::START_SERVICE_RESPONSE), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::STOP_SERVICE_REQUEST), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::STOP_SERVICE_RESPONSE), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::UPDATE_SERVICE_LIST_REQUEST), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::UPDATE_SERVICE_LIST_RESPONSE), nullptr);
}

// Test Initialize multiple times
TEST_F(EndPointFactoryTest, Initialize_MultipleCalls) {
    EndPointFactory factory;
    
    int32_t size1 = factory.Initialize();
    int32_t size2 = factory.Initialize();
    
    EXPECT_EQ(size1, 12);
    EXPECT_EQ(size2, 12);
}

// Test GetTopic without Initialize
TEST_F(EndPointFactoryTest, GetTopic_WithoutInitialize) {
    EndPointFactory factory;
    
    // Without Initialize, map is empty, should return nullptr
    EXPECT_EQ(factory.GetTopic(MessageType::LIFECYCLE_DIAG_REPORT), nullptr);
}

// Test SetCustomCreator for multiple message types
TEST_F(EndPointFactoryTest, SetCustomCreator_MultipleTimes) {
    EndPointFactory factory;
    
    auto creator1 = [](MessageType type, const std::string& topic) {
        return std::make_shared<MockDDSMessageEndPoint>(topic);
    };
    
    auto creator2 = [](MessageType type, const std::string& topic) {
        return std::make_shared<MockDDSMessageEndPoint>(topic);
    };
    
    factory.SetCustomCreator(MessageType::LIFECYCLE_DIAG_REPORT, creator1);
    factory.SetCustomCreator(MessageType::START_SERVICE_REQUEST, creator2);
    
    factory.Initialize();
    
    // Just verify no crash
    EXPECT_EQ(factory.Initialize(), 12);
}

// Test destructor
TEST_F(EndPointFactoryTest, Destructor_DoesNotCrash) {
    EndPointFactory* factory = new EndPointFactory();
    factory->Initialize();
    EXPECT_NO_THROW(delete factory);
}

// Test default constructor
TEST_F(EndPointFactoryTest, DefaultConstructor) {
    EndPointFactory factory;
    EXPECT_EQ(factory.GetTopic(MessageType::LIFECYCLE_DIAG_REPORT), nullptr);
}

// ==================== CreateEndPoints Branch Coverage ====================

// Test CreateEndPoints with all PubSubTypes (PUBLISH_ONLY, SUBSCRIBE_ONLY, PUBSUB_BOTH)
TEST_F(EndPointFactoryTest, CreateEndPoints_AllPubSubTypes) {
    EndPointFactory factory;
    auto transmitter = std::make_shared<DDSTransmitter>();
    
    // Setup mock expectations for transmitter methods
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    factory.Initialize();
    
    // This will exercise all switch cases and PubSubType branches
    EXPECT_NO_THROW(factory.CreateEndPoints(transmitter));
}

// Test CreateEndPoints with custom creator (covers PUBSUB_BOTH branch)
TEST_F(EndPointFactoryTest, CreateEndPoints_CustomCreatorPubSubBoth) {
    EndPointFactory factory;
    auto transmitter = std::make_shared<DDSTransmitter>();
    
    bool custom_creator_called = false;
    auto custom_creator = [&custom_creator_called](MessageType type, const std::string& topic) {
        custom_creator_called = true;
        auto mock = new MockDDSMessageEndPoint(topic);
        Mock_DDSMessageEndPoint = mock;
        return std::shared_ptr<MockDDSMessageEndPoint>(mock, [](MockDDSMessageEndPoint* p) {
            // Custom deleter to avoid double-free
        });
    };
    
    // Set custom creator for one endpoint type
    factory.SetCustomCreator(MessageType::LIFECYCLE_DIAG_REPORT, custom_creator);
    
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    factory.Initialize();
    factory.CreateEndPoints(transmitter);
    
    EXPECT_TRUE(custom_creator_called);
}

// Test CreateEndPoints with nullptr transmitter components
TEST_F(EndPointFactoryTest, CreateEndPoints_NullTransmitterComponents) {
    EndPointFactory factory;
    auto transmitter = std::make_shared<DDSTransmitter>();
    
    // Return nullptr for all transmitter components
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    factory.Initialize();
    
    // Should still work with null components (Initialize handles them)
    int32_t size = factory.CreateEndPoints(transmitter);
    EXPECT_EQ(size, 12);
}

// Test CreateEndPoints returns correct size
TEST_F(EndPointFactoryTest, CreateEndPoints_ReturnsCorrectSize) {
    EndPointFactory factory;
    auto transmitter = std::make_shared<DDSTransmitter>();
    
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    factory.Initialize();
    int32_t size = factory.CreateEndPoints(transmitter);
    
    // 12 message types configured
    EXPECT_EQ(size, 12);
}

// Test CreateEndPoints without Initialize (empty topic_lists_)
TEST_F(EndPointFactoryTest, CreateEndPoints_WithoutInitialize) {
    EndPointFactory factory;
    auto transmitter = std::make_shared<DDSTransmitter>();
    
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    
    // CreateEndPoints without Initialize - should return 0
    int32_t size = factory.CreateEndPoints(transmitter);
    EXPECT_EQ(size, 0);
}

// ==================== Endpoint Constructor Branch Coverage ====================

// Test StartServiceEndPoint with REQUEST type
TEST_F(EndPointFactoryTest, StartServiceEndPoint_RequestType) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test StartServiceEndPoint with RESPONSE type
TEST_F(EndPointFactoryTest, StartServiceEndPoint_ResponseType) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_RESPONSE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test StartServiceEndPoint with invalid type (neither REQUEST nor RESPONSE)
TEST_F(EndPointFactoryTest, StartServiceEndPoint_InvalidType) {
    // Using a different message type - TypeSupport should not be set
    StartServiceEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    // TypeSupport may be nullptr or set differently
}

// Test StopServiceEndPoint with REQUEST type
TEST_F(EndPointFactoryTest, StopServiceEndPoint_RequestType) {
    StopServiceEndPoint endpoint(MessageType::STOP_SERVICE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test StopServiceEndPoint with RESPONSE type
TEST_F(EndPointFactoryTest, StopServiceEndPoint_ResponseType) {
    StopServiceEndPoint endpoint(MessageType::STOP_SERVICE_RESPONSE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test StopServiceEndPoint with invalid type
TEST_F(EndPointFactoryTest, StopServiceEndPoint_InvalidType) {
    StopServiceEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// Test GetServiceStateEndPoint with REQUEST type
TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_RequestType) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test GetServiceStateEndPoint with RESPONSE type
TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_ResponseType) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_RESPONSE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test GetServiceStateEndPoint with invalid type
TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_InvalidType) {
    GetServiceStateEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// Test NotifyServiceStateEndPoint with correct type
TEST_F(EndPointFactoryTest, NotifyServiceStateEndPoint_CorrectType) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test NotifyServiceStateEndPoint with invalid type
TEST_F(EndPointFactoryTest, NotifyServiceStateEndPoint_InvalidType) {
    NotifyServiceStateEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// Test SetServiceModeEndPoint with REQUEST type
TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_RequestType) {
    SetServiceModeEndPoint endpoint(MessageType::SET_SERVICE_MODE_REQUST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test SetServiceModeEndPoint with RESPONSE type
TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_ResponseType) {
    SetServiceModeEndPoint endpoint(MessageType::SET_SERVICE_MODE_RESPONSE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test SetServiceModeEndPoint with invalid type
TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_InvalidType) {
    SetServiceModeEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// Test UpdateServiceListEndPoint with REQUEST type
TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_RequestType) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test UpdateServiceListEndPoint with RESPONSE type
TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_ResponseType) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test UpdateServiceListEndPoint with invalid type
TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_InvalidType) {
    UpdateServiceListEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// Test LifecycleDiagReportEndPoint with correct type
TEST_F(EndPointFactoryTest, LifecycleDiagReportEndPoint_CorrectType) {
    LifecycleDiagReportEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

// Test LifecycleDiagReportEndPoint with invalid type
TEST_F(EndPointFactoryTest, LifecycleDiagReportEndPoint_InvalidType) {
    LifecycleDiagReportEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
}

// ==================== Endpoint Destructor Coverage ====================

TEST_F(EndPointFactoryTest, StartServiceEndPoint_Destructor) {
    StartServiceEndPoint* endpoint = new StartServiceEndPoint(MessageType::START_SERVICE_REQUEST, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, StopServiceEndPoint_Destructor) {
    StopServiceEndPoint* endpoint = new StopServiceEndPoint(MessageType::STOP_SERVICE_REQUEST, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_Destructor) {
    GetServiceStateEndPoint* endpoint = new GetServiceStateEndPoint(MessageType::GET_SERVICE_STATE_REQUEST, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, NotifyServiceStateEndPoint_Destructor) {
    NotifyServiceStateEndPoint* endpoint = new NotifyServiceStateEndPoint(MessageType::NOTIFY_SERVICE_STATE, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_Destructor) {
    SetServiceModeEndPoint* endpoint = new SetServiceModeEndPoint(MessageType::SET_SERVICE_MODE_REQUST, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_Destructor) {
    UpdateServiceListEndPoint* endpoint = new UpdateServiceListEndPoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test");
    EXPECT_NO_THROW(delete endpoint);
}

TEST_F(EndPointFactoryTest, LifecycleDiagReportEndPoint_Destructor) {
    LifecycleDiagReportEndPoint* endpoint = new LifecycleDiagReportEndPoint(MessageType::LIFECYCLE_DIAG_REPORT, "test");
    EXPECT_NO_THROW(delete endpoint);
}

// ==================== OnDataPublished Branch Coverage ====================

TEST_F(EndPointFactoryTest, StartServiceEndPoint_OnDataPublished_Success) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, StartServiceEndPoint_OnDataPublished_Failure) {
    StartServiceEndPoint endpoint(MessageType::START_SERVICE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, StopServiceEndPoint_OnDataPublished_Success) {
    StopServiceEndPoint endpoint(MessageType::STOP_SERVICE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, StopServiceEndPoint_OnDataPublished_Failure) {
    StopServiceEndPoint endpoint(MessageType::STOP_SERVICE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_OnDataPublished_Success) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, GetServiceStateEndPoint_OnDataPublished_Failure) {
    GetServiceStateEndPoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, NotifyServiceStateEndPoint_OnDataPublished_Success) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, NotifyServiceStateEndPoint_OnDataPublished_Failure) {
    NotifyServiceStateEndPoint endpoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_OnDataPublished_Success) {
    SetServiceModeEndPoint endpoint(MessageType::SET_SERVICE_MODE_REQUST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, SetServiceModeEndPoint_OnDataPublished_Failure) {
    SetServiceModeEndPoint endpoint(MessageType::SET_SERVICE_MODE_REQUST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_OnDataPublished_Success) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, UpdateServiceListEndPoint_OnDataPublished_Failure) {
    UpdateServiceListEndPoint endpoint(MessageType::UPDATE_SERVICE_LIST_REQUEST, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

TEST_F(EndPointFactoryTest, LifecycleDiagReportEndPoint_OnDataPublished_Success) {
    LifecycleDiagReportEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(true));
}

TEST_F(EndPointFactoryTest, LifecycleDiagReportEndPoint_OnDataPublished_Failure) {
    LifecycleDiagReportEndPoint endpoint(MessageType::LIFECYCLE_DIAG_REPORT, "test_topic");
    EXPECT_NO_THROW(endpoint.OnDataPublished(false));
}

// ==================== GetTopic Edge Cases ====================

// Test GetTopic with boundary message types
TEST_F(EndPointFactoryTest, GetTopic_BoundaryMessageTypes) {
    EndPointFactory factory;
    factory.Initialize();
    
    // Test with first and last valid message types
    EXPECT_NE(factory.GetTopic(MessageType::LIFECYCLE_DIAG_REPORT), nullptr);
    EXPECT_NE(factory.GetTopic(MessageType::UPDATE_SERVICE_LIST_RESPONSE), nullptr);
}

// Test GetTopic returns correct strings
TEST_F(EndPointFactoryTest, GetTopic_CorrectStrings) {
    EndPointFactory factory;
    factory.Initialize();
    
    EXPECT_STREQ(factory.GetTopic(MessageType::GET_SERVICE_STATE_REQUEST), "mmf/lifecyclemgr/GetServiceStateRequest");
    EXPECT_STREQ(factory.GetTopic(MessageType::GET_SERVICE_STATE_RESPONSE), "mmf/lifecyclemgr/GetServiceStateResponse");
    EXPECT_STREQ(factory.GetTopic(MessageType::NOTIFY_SERVICE_STATE), "mmf/lifecyclemgr/NotifyServiceState");
    EXPECT_STREQ(factory.GetTopic(MessageType::START_SERVICE_REQUEST), "mmf/lifecyclemgr/StartServiceRequest");
    EXPECT_STREQ(factory.GetTopic(MessageType::START_SERVICE_RESPONSE), "mmf/lifecyclemgr/StartServiceResponse");
    EXPECT_STREQ(factory.GetTopic(MessageType::STOP_SERVICE_REQUEST), "mmf/lifecyclemgr/StopServiceRequest");
    EXPECT_STREQ(factory.GetTopic(MessageType::STOP_SERVICE_RESPONSE), "mmf/lifecyclemgr/StopServiceResponse");
    EXPECT_STREQ(factory.GetTopic(MessageType::UPDATE_SERVICE_LIST_REQUEST), "mmf/lifecyclemgr/UpdateServiceListRequest");
}

// ==================== SetCustomCreator Edge Cases ====================

// Test SetCustomCreator overwrite existing
TEST_F(EndPointFactoryTest, SetCustomCreator_OverwriteExisting) {
    EndPointFactory factory;
    
    int call_count = 0;
    auto creator1 = [&call_count](MessageType type, const std::string& topic) {
        call_count = 1;
        return std::make_shared<MockDDSMessageEndPoint>(topic);
    };
    auto creator2 = [&call_count](MessageType type, const std::string& topic) {
        call_count = 2;
        return std::make_shared<MockDDSMessageEndPoint>(topic);
    };
    
    factory.SetCustomCreator(MessageType::LIFECYCLE_DIAG_REPORT, creator1);
    factory.SetCustomCreator(MessageType::LIFECYCLE_DIAG_REPORT, creator2); // Overwrite
    
    factory.Initialize();
    
    auto transmitter = std::make_shared<DDSTransmitter>();
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    factory.CreateEndPoints(transmitter);
    
    // Second creator should have been used
    EXPECT_EQ(call_count, 2);
}

// Test SetCustomCreator for all message types
TEST_F(EndPointFactoryTest, SetCustomCreator_AllMessageTypes) {
    EndPointFactory factory;
    
    auto custom_creator = [](MessageType type, const std::string& topic) {
        auto mock = new MockDDSMessageEndPoint(topic);
        Mock_DDSMessageEndPoint = mock;
        return std::shared_ptr<MockDDSMessageEndPoint>(mock, [](MockDDSMessageEndPoint* p) {});
    };
    
    // Set custom creators for all types
    factory.SetCustomCreator(MessageType::LIFECYCLE_DIAG_REPORT, custom_creator);
    factory.SetCustomCreator(MessageType::GET_SERVICE_STATE_REQUEST, custom_creator);
    factory.SetCustomCreator(MessageType::GET_SERVICE_STATE_RESPONSE, custom_creator);
    factory.SetCustomCreator(MessageType::NOTIFY_SERVICE_STATE, custom_creator);
    factory.SetCustomCreator(MessageType::SET_SERVICE_MODE_REQUST, custom_creator);
    factory.SetCustomCreator(MessageType::SET_SERVICE_MODE_RESPONSE, custom_creator);
    factory.SetCustomCreator(MessageType::START_SERVICE_REQUEST, custom_creator);
    factory.SetCustomCreator(MessageType::START_SERVICE_RESPONSE, custom_creator);
    factory.SetCustomCreator(MessageType::STOP_SERVICE_REQUEST, custom_creator);
    factory.SetCustomCreator(MessageType::STOP_SERVICE_RESPONSE, custom_creator);
    factory.SetCustomCreator(MessageType::UPDATE_SERVICE_LIST_REQUEST, custom_creator);
    factory.SetCustomCreator(MessageType::UPDATE_SERVICE_LIST_RESPONSE, custom_creator);
    
    factory.Initialize();
    
    auto transmitter = std::make_shared<DDSTransmitter>();
    ON_CALL(*Mock_DDSTransmitter, GetDomain()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetPublisher()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, GetSubscriber()).WillByDefault(Return(nullptr));
    ON_CALL(*Mock_DDSTransmitter, AddMessageEndpoint(_)).WillByDefault(Return(true));
    ON_CALL(*Mock_DDSMessageEndPoint, Initialize(_, _, _)).WillByDefault(Return(true));
    
    EXPECT_NO_THROW(factory.CreateEndPoints(transmitter));
}

// ==================== Endpoint GetTopic Coverage ====================

TEST_F(EndPointFactoryTest, AllEndpoints_GetTopic) {
    {
        StartServiceEndPoint ep(MessageType::START_SERVICE_REQUEST, "start_topic");
        EXPECT_EQ(ep.GetTopic(), "start_topic");
    }
    {
        StopServiceEndPoint ep(MessageType::STOP_SERVICE_REQUEST, "stop_topic");
        EXPECT_EQ(ep.GetTopic(), "stop_topic");
    }
    {
        GetServiceStateEndPoint ep(MessageType::GET_SERVICE_STATE_REQUEST, "get_state_topic");
        EXPECT_EQ(ep.GetTopic(), "get_state_topic");
    }
    {
        NotifyServiceStateEndPoint ep(MessageType::NOTIFY_SERVICE_STATE, "notify_topic");
        EXPECT_EQ(ep.GetTopic(), "notify_topic");
    }
    {
        SetServiceModeEndPoint ep(MessageType::SET_SERVICE_MODE_REQUST, "set_mode_topic");
        EXPECT_EQ(ep.GetTopic(), "set_mode_topic");
    }
    {
        UpdateServiceListEndPoint ep(MessageType::UPDATE_SERVICE_LIST_REQUEST, "update_list_topic");
        EXPECT_EQ(ep.GetTopic(), "update_list_topic");
    }
    {
        LifecycleDiagReportEndPoint ep(MessageType::LIFECYCLE_DIAG_REPORT, "diag_topic");
        EXPECT_EQ(ep.GetTopic(), "diag_topic");
    }
}

// ==================== Endpoint GetTypeSupport Coverage ====================

TEST_F(EndPointFactoryTest, AllEndpoints_GetTypeSupport) {
    {
        StartServiceEndPoint ep(MessageType::START_SERVICE_REQUEST, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        StartServiceEndPoint ep(MessageType::START_SERVICE_RESPONSE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        StopServiceEndPoint ep(MessageType::STOP_SERVICE_REQUEST, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        StopServiceEndPoint ep(MessageType::STOP_SERVICE_RESPONSE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        GetServiceStateEndPoint ep(MessageType::GET_SERVICE_STATE_REQUEST, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        GetServiceStateEndPoint ep(MessageType::GET_SERVICE_STATE_RESPONSE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        NotifyServiceStateEndPoint ep(MessageType::NOTIFY_SERVICE_STATE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        SetServiceModeEndPoint ep(MessageType::SET_SERVICE_MODE_REQUST, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        SetServiceModeEndPoint ep(MessageType::SET_SERVICE_MODE_RESPONSE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        UpdateServiceListEndPoint ep(MessageType::UPDATE_SERVICE_LIST_REQUEST, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        UpdateServiceListEndPoint ep(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
    {
        LifecycleDiagReportEndPoint ep(MessageType::LIFECYCLE_DIAG_REPORT, "topic");
        EXPECT_NE(ep.GetTypeSupport(), nullptr);
    }
}

// ==================== Endpoint GetTypeName Coverage ====================

TEST_F(EndPointFactoryTest, AllEndpoints_GetTypeName) {
    {
        StartServiceEndPoint ep(MessageType::START_SERVICE_REQUEST, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        StopServiceEndPoint ep(MessageType::STOP_SERVICE_REQUEST, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        GetServiceStateEndPoint ep(MessageType::GET_SERVICE_STATE_REQUEST, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        NotifyServiceStateEndPoint ep(MessageType::NOTIFY_SERVICE_STATE, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        SetServiceModeEndPoint ep(MessageType::SET_SERVICE_MODE_REQUST, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        UpdateServiceListEndPoint ep(MessageType::UPDATE_SERVICE_LIST_REQUEST, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
    {
        LifecycleDiagReportEndPoint ep(MessageType::LIFECYCLE_DIAG_REPORT, "topic");
        EXPECT_FALSE(ep.GetTypeName().empty());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
