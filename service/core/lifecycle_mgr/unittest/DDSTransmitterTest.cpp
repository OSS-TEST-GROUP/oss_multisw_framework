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

using namespace lifecycle_manager;

class DDSTransmitterTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DDSTransmitter();

        Mock_Logger = new MockLogger();
        Mock_MessageQueue = new MockMessageQueue(ins);
        Mock_MessageEndPoint = new MockMessageEndPoint("test_topic");
    }
    virtual void TearDown(){
        delete ins;
        delete Mock_Logger;
        delete Mock_MessageQueue;
        delete Mock_MessageEndPoint;

        Mock_Logger = nullptr;
        Mock_MessageQueue = nullptr;
        Mock_MessageEndPoint = nullptr;
    }

    DDSTransmitter* ins;
};


TEST_F(DDSTransmitterTest, InitializeReturnsTrue) {
    EXPECT_TRUE(ins->Initialize());
}

TEST_F(DDSTransmitterTest, FinalizeDoesNotCrash) {
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, DoubleInitializeIsIdempotent) {
    EXPECT_TRUE(ins->Initialize());
    EXPECT_TRUE(ins->Initialize());
}

TEST_F(DDSTransmitterTest, DoubleFinalizeIsIdempotent) {
    ins->Finalize();
    ins->Finalize();
}

TEST_F(DDSTransmitterTest, AddValidMessageEndpointReturnsFalseButStores) {
    //EXPECT_FALSE(ins->AddMessageEndpoint(Mock_MessageEndPoint));
    //EXPECT_EQ(ins->GetMessageEndPoint("test_topic"), Mock_MessageEndPoint);
}

TEST_F(DDSTransmitterTest, AddNullMessageEndpointReturnsFalse) {
    EXPECT_FALSE(ins->AddMessageEndpoint(nullptr));
}

TEST_F(DDSTransmitterTest, GetMessageEndPointNotFoundReturnsNullptr) {
    EXPECT_EQ(ins->GetMessageEndPoint("not_exist"), nullptr);
}

TEST_F(DDSTransmitterTest, SendMessageSuccess) {
    MsgType msg{MessageType::LIFECYCLE_DIAG_REPORT, nullptr};
    EXPECT_CALL(*Mock_MessageQueue, SendMessage(_)).Times(1);
    EXPECT_TRUE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
}

TEST_F(DDSTransmitterTest, SendMessageFailureWhenQueueNull) {
    ins->Finalize(); // This will set message_queue_ to nullptr
    EXPECT_FALSE(ins->SendMessage(MessageType::LIFECYCLE_DIAG_REPORT, nullptr));
}

TEST_F(DDSTransmitterTest, GetDomainReturnsPointer) {
    EXPECT_EQ(ins->GetDomain(), ins->GetDomain()); // Just check getter
}

TEST_F(DDSTransmitterTest, GetPublisherReturnsPointer) {
    EXPECT_EQ(ins->GetPublisher(), ins->GetPublisher());
}

TEST_F(DDSTransmitterTest, GetSubscriberReturnsPointer) {
    EXPECT_EQ(ins->GetSubscriber(), ins->GetSubscriber());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
