// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "transmitter/DDSTransmitter.hpp"
#include "unittest/mock/mock_dds_message_endpoint.hpp"
#include "unittest/mock/mock_message_endpoint.hpp"  
#include "unittest/mock/mock_message_queue.hpp"

#include "unittest/mock/mock_logger.hpp"

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

using namespace lifecycle_manager;

class DDSTransmitterTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DDSTransmitter();

        Mock_Logger = new MockLogger();
        Mock_MessageQueue = new NiceMock<MockMessageQueue>(ins);
        MockMessageEndPoint::Mock_MessageEndPoint = new NiceMock<MockMessageEndPoint>("test_topic");
        
        // Set default behaviors for mocks
        ON_CALL(*Mock_MessageQueue, Start()).WillByDefault(Return(true));
        ON_CALL(*Mock_MessageQueue, SendMessage(_)).WillByDefault(Return());
    }
    virtual void TearDown(){
        delete ins;
        delete Mock_Logger;
        delete Mock_MessageQueue;
        delete MockMessageEndPoint::Mock_MessageEndPoint;

        Mock_Logger = nullptr;
        Mock_MessageQueue = nullptr;
        MockMessageEndPoint::Mock_MessageEndPoint = nullptr;
    }

    DDSTransmitter* ins;
};


TEST_F(DDSTransmitterTest, InitializeReturnsTrue) {
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();  // Clean up after Initialize
}

TEST_F(DDSTransmitterTest, FinalizeDoesNotCrash) {
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, DoubleInitializeIsIdempotent) {
    EXPECT_TRUE(ins->Initialize());
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, DoubleFinalizeIsIdempotent) {
    ins->Finalize();
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, FinalizeAfterInitialize) {
    // Test the participant_ != nullptr branch in Finalize
    EXPECT_TRUE(ins->Initialize());
    EXPECT_NO_THROW(ins->Finalize());
}

TEST_F(DDSTransmitterTest, AddValidMessageEndpointStoresEndpoint) {
    auto mockEndpoint = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("test_topic");
    Mock_DDSMessageEndPoint = mockEndpoint.get();
    
    ON_CALL(*mockEndpoint, GetTopic()).WillByDefault(Return("test_topic"));
    
    // AddMessageEndpoint returns false but should store the endpoint
    ins->AddMessageEndpoint(mockEndpoint);
    
    // The endpoint should be retrievable
    auto result = ins->GetMessageEndPoint("test_topic");
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, mockEndpoint);
}

TEST_F(DDSTransmitterTest, AddNullMessageEndpointReturnsFalse) {
    EXPECT_FALSE(ins->AddMessageEndpoint(nullptr));
}

TEST_F(DDSTransmitterTest, GetMessageEndPointNotFoundReturnsNullptr) {
    EXPECT_EQ(ins->GetMessageEndPoint("not_exist"), nullptr);
}

TEST_F(DDSTransmitterTest, GetMessageEndPointFound) {
    auto mockEndpoint = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("found_topic");
    Mock_DDSMessageEndPoint = mockEndpoint.get();
    
    ON_CALL(*mockEndpoint, GetTopic()).WillByDefault(Return("found_topic"));
    
    ins->AddMessageEndpoint(mockEndpoint);
    
    // Now test the found case
    auto result = ins->GetMessageEndPoint("found_topic");
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, mockEndpoint);
}

TEST_F(DDSTransmitterTest, SendMessageSuccessAfterInitialize) {
    EXPECT_TRUE(ins->Initialize());
    
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(1);
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, SendMessageFailureWhenNotInitialized) {
    // Without Initialize, message_queue_ is nullptr
    EXPECT_FALSE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
}

TEST_F(DDSTransmitterTest, SendMessageWithDifferentMessageTypes) {
    EXPECT_TRUE(ins->Initialize());
    
    int dummy_data = 42;
    
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(3);
    
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
    EXPECT_TRUE(ins->SendMessage(MessageType::START_SERVICE_REQUEST, &dummy_data));
    EXPECT_TRUE(ins->SendMessage(MessageType::STOP_SERVICE_REQUEST, &dummy_data));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, GetDomainBeforeInitialize) {
    EXPECT_EQ(ins->GetDomain(), nullptr);
}

TEST_F(DDSTransmitterTest, GetDomainAfterInitialize) {
    EXPECT_TRUE(ins->Initialize());
    EXPECT_NE(ins->GetDomain(), nullptr);
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, GetPublisherBeforeInitialize) {
    EXPECT_EQ(ins->GetPublisher(), nullptr);
}

TEST_F(DDSTransmitterTest, GetPublisherAfterInitialize) {
    EXPECT_TRUE(ins->Initialize());
    EXPECT_NE(ins->GetPublisher(), nullptr);
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, GetSubscriberBeforeInitialize) {
    EXPECT_EQ(ins->GetSubscriber(), nullptr);
}

TEST_F(DDSTransmitterTest, GetSubscriberAfterInitialize) {
    EXPECT_TRUE(ins->Initialize());
    EXPECT_NE(ins->GetSubscriber(), nullptr);
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, AddMultipleEndpoints) {
    auto endpoint1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("topic1");
    auto endpoint2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("topic2");
    
    ON_CALL(*endpoint1, GetTopic()).WillByDefault(Return("topic1"));
    ON_CALL(*endpoint2, GetTopic()).WillByDefault(Return("topic2"));
    
    Mock_DDSMessageEndPoint = endpoint1.get();
    ins->AddMessageEndpoint(endpoint1);
    
    Mock_DDSMessageEndPoint = endpoint2.get();
    ins->AddMessageEndpoint(endpoint2);
    
    EXPECT_NE(ins->GetMessageEndPoint("topic1"), nullptr);
    EXPECT_NE(ins->GetMessageEndPoint("topic2"), nullptr);
}

// ==================== Additional Coverage Tests ====================

TEST_F(DDSTransmitterTest, SendMessage_WithValidData) {
    EXPECT_TRUE(ins->Initialize());
    
    // Test with actual data pointer
    int test_data = 123;
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(1);
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, &test_data));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, GetMessageEndPoint_AfterAddingMultiple) {
    auto ep1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("topic_a");
    auto ep2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("topic_b");
    auto ep3 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("topic_c");
    
    ON_CALL(*ep1, GetTopic()).WillByDefault(Return("topic_a"));
    ON_CALL(*ep2, GetTopic()).WillByDefault(Return("topic_b"));
    ON_CALL(*ep3, GetTopic()).WillByDefault(Return("topic_c"));
    
    Mock_DDSMessageEndPoint = ep1.get();
    ins->AddMessageEndpoint(ep1);
    
    Mock_DDSMessageEndPoint = ep2.get();
    ins->AddMessageEndpoint(ep2);
    
    Mock_DDSMessageEndPoint = ep3.get();
    ins->AddMessageEndpoint(ep3);
    
    // Test found cases (covers the if branch in GetMessageEndPoint)
    auto found_a = ins->GetMessageEndPoint("topic_a");
    auto found_b = ins->GetMessageEndPoint("topic_b");
    auto found_c = ins->GetMessageEndPoint("topic_c");
    
    EXPECT_NE(found_a, nullptr);
    EXPECT_NE(found_b, nullptr);
    EXPECT_NE(found_c, nullptr);
    
    // Test not found case (covers the else branch)
    auto not_found = ins->GetMessageEndPoint("nonexistent_topic");
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(DDSTransmitterTest, AddMessageEndpoint_OverwriteExisting) {
    auto ep1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("same_topic");
    auto ep2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("same_topic");
    
    ON_CALL(*ep1, GetTopic()).WillByDefault(Return("same_topic"));
    ON_CALL(*ep2, GetTopic()).WillByDefault(Return("same_topic"));
    
    Mock_DDSMessageEndPoint = ep1.get();
    ins->AddMessageEndpoint(ep1);
    
    Mock_DDSMessageEndPoint = ep2.get();
    ins->AddMessageEndpoint(ep2);
    
    // Should have overwritten with ep2
    auto found = ins->GetMessageEndPoint("same_topic");
    EXPECT_EQ(found, ep2);
}

TEST_F(DDSTransmitterTest, Initialize_MessageQueueNotNull) {
    // After Initialize, message_queue_ should be set
    EXPECT_TRUE(ins->Initialize());
    
    // SendMessage should work (indicating message_queue_ is not null)
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(1);
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, SendMessage_LogsFailureWhenQueueNull) {
    // Without Initialize, message_queue_ is nullptr
    // This covers the LOG_DEBUG failure path
    EXPECT_FALSE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
    EXPECT_FALSE(ins->SendMessage(MessageType::START_SERVICE_REQUEST, nullptr));
    EXPECT_FALSE(ins->SendMessage(MessageType::STOP_SERVICE_REQUEST, nullptr));
}

TEST_F(DDSTransmitterTest, GettersAfterFinalize) {
    EXPECT_TRUE(ins->Initialize());
    ins->Finalize();
    
    // After Finalize, participant_ is deleted but the pointer may still exist
    // Just ensure no crash
    EXPECT_NO_THROW({
        ins->GetDomain();
        ins->GetPublisher();
        ins->GetSubscriber();
    });
}

// ============================================================================
// Enhanced Coverage Tests - Error Paths and Edge Cases
// ============================================================================

TEST_F(DDSTransmitterTest, AddMessageEndpoint_DuplicateTopic_TC001) {
    auto endpoint1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("SameTopic");
    auto endpoint2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("SameTopic");
    
    // Both endpoints have same topic
    ON_CALL(*endpoint1, GetTopic()).WillByDefault(Return("SameTopic"));
    ON_CALL(*endpoint2, GetTopic()).WillByDefault(Return("SameTopic"));
    
    Mock_DDSMessageEndPoint = endpoint1.get();
    ins->AddMessageEndpoint(endpoint1);
    
    // Adding second endpoint with same topic should overwrite
    Mock_DDSMessageEndPoint = endpoint2.get();
    ins->AddMessageEndpoint(endpoint2);
    
    ins->Initialize();
    
    // Should get the second endpoint
    auto ret = ins->GetMessageEndPoint("SameTopic");
    EXPECT_EQ(ret, endpoint2);
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, Initialize_ThenAddEndpoint_TC002) {
    // Initialize transmitter first
    EXPECT_TRUE(ins->Initialize());
    
    // Then add endpoint
    auto endpoint = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("LateAddedTopic");
    ON_CALL(*endpoint, GetTopic()).WillByDefault(Return("LateAddedTopic"));
    Mock_DDSMessageEndPoint = endpoint.get();
    
    ins->AddMessageEndpoint(endpoint);
    
    // Should be able to retrieve it
    auto ret = ins->GetMessageEndPoint("LateAddedTopic");
    EXPECT_EQ(ret, endpoint);
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, Finalize_WithMultipleEndpoints_TC003) {
    // Add multiple endpoints
    auto endpoint1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic1");
    auto endpoint2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic2");
    auto endpoint3 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic3");
    
    ON_CALL(*endpoint1, GetTopic()).WillByDefault(Return("Topic1"));
    ON_CALL(*endpoint2, GetTopic()).WillByDefault(Return("Topic2"));
    ON_CALL(*endpoint3, GetTopic()).WillByDefault(Return("Topic3"));
    
    ON_CALL(*endpoint1, Finalize()).WillByDefault(Return());
    ON_CALL(*endpoint2, Finalize()).WillByDefault(Return());
    ON_CALL(*endpoint3, Finalize()).WillByDefault(Return());
    
    Mock_DDSMessageEndPoint = endpoint1.get();
    ins->AddMessageEndpoint(endpoint1);
    
    Mock_DDSMessageEndPoint = endpoint2.get();
    ins->AddMessageEndpoint(endpoint2);
    
    Mock_DDSMessageEndPoint = endpoint3.get();
    ins->AddMessageEndpoint(endpoint3);
    
    ins->Initialize();
    
    // Finalize should call Finalize on all endpoints
    EXPECT_CALL(*endpoint1, Finalize()).Times(testing::AtLeast(1));
    EXPECT_CALL(*endpoint2, Finalize()).Times(testing::AtLeast(1));
    EXPECT_CALL(*endpoint3, Finalize()).Times(testing::AtLeast(1));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, SendMessage_AllTypesInSequence_TC004) {
    EXPECT_TRUE(ins->Initialize());
    
    int data1 = 1, data2 = 2, data3 = 3;
    
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(3);
    
    // Send different message types in sequence
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, &data1));
    EXPECT_TRUE(ins->SendMessage(MessageType::START_SERVICE_REQUEST, &data2));
    EXPECT_TRUE(ins->SendMessage(MessageType::STOP_SERVICE_REQUEST, &data3));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, SendMessage_RapidFire_TC005) {
    EXPECT_TRUE(ins->Initialize());
    
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(100);
    
    // Send many messages rapidly
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(ins->SendMessage(
            MessageType::LIFECYCLE_DIAG_REPORT, 
            reinterpret_cast<void*>(static_cast<uintptr_t>(i))
        ));
    }
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, MultipleEndpoints_Retrieval_TC006) {
    // Test adding endpoints and verifying map storage
    auto endpoint1 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic_A");
    auto endpoint2 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic_B");
    auto endpoint3 = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("Topic_C");
    
    ON_CALL(*endpoint1, GetTopic()).WillByDefault(Return("Topic_A"));
    ON_CALL(*endpoint2, GetTopic()).WillByDefault(Return("Topic_B"));
    ON_CALL(*endpoint3, GetTopic()).WillByDefault(Return("Topic_C"));
    
    Mock_DDSMessageEndPoint = endpoint1.get();
    ins->AddMessageEndpoint(endpoint1);
    
    Mock_DDSMessageEndPoint = endpoint2.get();
    ins->AddMessageEndpoint(endpoint2);
    
    Mock_DDSMessageEndPoint = endpoint3.get();
    ins->AddMessageEndpoint(endpoint3);
    
    ins->Initialize();
    
    // Verify all can be retrieved
    EXPECT_EQ(ins->GetMessageEndPoint("Topic_A"), endpoint1);
    EXPECT_EQ(ins->GetMessageEndPoint("Topic_B"), endpoint2);
    EXPECT_EQ(ins->GetMessageEndPoint("Topic_C"), endpoint3);
    
    // Verify non-existent returns nullptr
    EXPECT_EQ(ins->GetMessageEndPoint("Topic_D"), nullptr);
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, GettersBeforeInitialize_TC007) {
    // Before initialization, getters should return nullptr
    EXPECT_TRUE(ins->GetDomain() == nullptr);
    EXPECT_TRUE(ins->GetPublisher() == nullptr);
    EXPECT_TRUE(ins->GetSubscriber() == nullptr);
}

TEST_F(DDSTransmitterTest, InitializeAfterFinalize_TC008) {
    ins->Initialize();
    ins->Finalize();
    
    // Should be able to re-initialize
    EXPECT_TRUE(ins->Initialize());
    EXPECT_NE(ins->GetDomain(), nullptr);
    EXPECT_NE(ins->GetPublisher(), nullptr);
    EXPECT_NE(ins->GetSubscriber(), nullptr);
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, FinalizeBeforeInitialize_TC009) {
    // Finalize without initialization should not crash
    EXPECT_NO_THROW(ins->Finalize());
}

TEST_F(DDSTransmitterTest, SendMessage_WithNullPtr_TC010) {
    EXPECT_TRUE(ins->Initialize());
    
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(1);
    
    // SendMessage with nullptr should still work (queue handles it)
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
    
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, AddMessageEndpoint_EmptyTopicName_TC011) {
    auto endpoint = std::make_shared<NiceMock<MockDDSMessageEndPoint>>("");
    
    ON_CALL(*endpoint, GetTopic()).WillByDefault(Return(""));
    Mock_DDSMessageEndPoint = endpoint.get();
    
    // Should handle empty topic name
    ins->AddMessageEndpoint(endpoint);
    
    auto ret = ins->GetMessageEndPoint("");
    EXPECT_EQ(ret, endpoint);
}

TEST_F(DDSTransmitterTest, GetMessageEndPoint_BeforeAddingAny_TC012) {
    // Before adding any endpoints, all lookups should return nullptr
    EXPECT_EQ(ins->GetMessageEndPoint("AnyTopic"), nullptr);
    EXPECT_EQ(ins->GetMessageEndPoint(""), nullptr);
    EXPECT_EQ(ins->GetMessageEndPoint("test_topic"), nullptr);
}

// ============================================================================
// END Enhanced Coverage Tests
// ============================================================================


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
