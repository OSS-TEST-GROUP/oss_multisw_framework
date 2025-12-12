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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
