// Unit tests for EndPointFactory class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "endpoint/EndPointFactory.hpp"
#include "unittest/mock/mock_endpoint_factory.hpp"
#include "unittest/mock/mock_dds_transmitter.hpp"

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


class EndPointFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(EndPointFactoryTest, CreateEndPointsCalled) {
    MockEndPointFactory factory;
    auto dds = std::make_shared<MockDDSTransmitter>();
    EXPECT_CALL(factory, CreateEndPoints(dds)).Times(1);
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
    factory.Initialize();
    auto dds = std::make_shared<MockDDSTransmitter>();
    // Set up expectations for GetDomain, GetPublisher, GetSubscriber if needed
    EXPECT_CALL(*dds, GetDomain()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*dds, GetPublisher()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*dds, GetSubscriber()).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*dds, AddMessageEndpoint(_)).Times(12);
    int32_t size = factory.CreateEndPoints(dds);
    EXPECT_EQ(size, 12);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
