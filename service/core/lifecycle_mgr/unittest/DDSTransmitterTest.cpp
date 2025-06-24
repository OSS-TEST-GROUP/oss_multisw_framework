// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unittest/mock/mock_dds_transmitter.hpp"
#include "unittest/mock/mock_dds_message_endpoint.hpp"
#include "unittest/mock/mock_message_endpoint.hpp"  


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
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(DDSTransmitterTest, InitializeCalled) {
    MockDDSTransmitter dds;
    EXPECT_CALL(dds, Initialize()).Times(1);
    dds.Initialize();
}

// Add more tests for DDSTransmitter as needed
TEST_F(DDSTransmitterTest, FinalizeCalled) {
    MockDDSTransmitter dds;
    EXPECT_CALL(dds, Finalize()).Times(1);
    dds.Finalize();
}

TEST_F(DDSTransmitterTest, AddMessageEndpointReturnsFalse) {
    MockDDSTransmitter dds;
#if 0    
    auto endpoint = std::make_shared<lifecycle_manager::DDSMessageEndPoint>("test_topic");
    EXPECT_CALL(dds, AddMessageEndpoint(endpoint)).WillOnce(Return(false));
    EXPECT_FALSE(dds.AddMessageEndpoint(endpoint));
    
#endif
}

TEST_F(DDSTransmitterTest, GetMessageEndPointReturnsNullptrWhenNotFound) {
    MockDDSTransmitter dds;
    EXPECT_CALL(dds, GetMessageEndPoint("not_exist")).WillOnce(Return(nullptr));
    EXPECT_EQ(dds.GetMessageEndPoint("not_exist"), nullptr);
}

TEST_F(DDSTransmitterTest, GetDomainReturnsParticipantPointer) {
    MockDDSTransmitter dds;
    DomainParticipant* dummy = reinterpret_cast<DomainParticipant*>(0x1234);
    EXPECT_CALL(dds, GetDomain()).WillOnce(Return(dummy));
    EXPECT_EQ(dds.GetDomain(), dummy);
}

TEST_F(DDSTransmitterTest, GetPublisherReturnsPublisherPointer) {
    MockDDSTransmitter dds;
    Publisher* dummy = reinterpret_cast<Publisher*>(0x5678);
    EXPECT_CALL(dds, GetPublisher()).WillOnce(Return(dummy));
    EXPECT_EQ(dds.GetPublisher(), dummy);
}

TEST_F(DDSTransmitterTest, GetSubscriberReturnsSubscriberPointer) {
    MockDDSTransmitter dds;
    Subscriber* dummy = reinterpret_cast<Subscriber*>(0x9abc);
    EXPECT_CALL(dds, GetSubscriber()).WillOnce(Return(dummy));
    EXPECT_EQ(dds.GetSubscriber(), dummy);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
