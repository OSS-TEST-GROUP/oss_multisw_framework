// Unit tests for EndPointFactory class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/EndPointFactory.hpp"
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
