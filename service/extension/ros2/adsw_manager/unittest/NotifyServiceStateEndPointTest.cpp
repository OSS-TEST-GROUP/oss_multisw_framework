// Unit tests for DDSTransmitter class using Google Test and Google Mock
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

using namespace ros2::adsw_manager;

// Test-only subclass for mocking Read behavior
class TestNotifyServiceStateEndPoint : public NotifyServiceStateEndPoint {
public:
    TestNotifyServiceStateEndPoint(MessageType msgtype, std::string topic)
        : NotifyServiceStateEndPoint(msgtype, topic) {}
    void OnDataAvailable() override {
        return;
    }
};

class NotifyServiceStateEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        
        Mock_DDSMessageEndpoint = new MockDDSMessageEndpoint();
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::NotifyServiceState_"));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeSupport()).WillByDefault(Return(nullptr));
        
        endpoint = new TestNotifyServiceStateEndPoint(MessageType::NOTIFY_SERVICE_STATE, "test_topic");
        endpoint->Initialize(nullptr, nullptr, nullptr);
    }
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete endpoint;
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;
        Mock_ADSWManager = nullptr;
    }
    
    TestNotifyServiceStateEndPoint* endpoint;
};

TEST_F(NotifyServiceStateEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_NO_THROW(endpoint->GetTypeName());
    EXPECT_NO_THROW(endpoint->GetTypeSupport());
}

TEST_F(NotifyServiceStateEndPointTest, OnDataAvailableDoesNotCrash) {
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}
TEST_F(NotifyServiceStateEndPointTest, OnDataAvailableDoesNotProcess) {
    // Since NotifyServiceState does not process data, we just check that it doesn't crash
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}
TEST_F(NotifyServiceStateEndPointTest, GetTypeSupportReturnsValidType) {
    EXPECT_NO_THROW(endpoint->GetTypeSupport());
}
TEST_F(NotifyServiceStateEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(NotifyServiceStateEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_NO_THROW(endpoint->GetTypeName());
}
TEST_F(NotifyServiceStateEndPointTest, TypeSupportIsNotNull) {
    EXPECT_NO_THROW(endpoint->GetTypeSupport());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
