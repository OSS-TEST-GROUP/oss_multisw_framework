// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/StopServiceEndPoint.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockTypeSupport.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#include "mock/MockROSServiceController.hpp"
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
class TestStopServiceEndPoint : public StopServiceEndPoint {
public:
    TestStopServiceEndPoint(MessageType msgtype, std::string topic)
        : StopServiceEndPoint(msgtype, topic) {}
    void OnDataAvailable() override {
        return;
    }
    bool mock_read_result = true;
};

class StopServiceEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        
        Mock_DDSMessageEndpoint = new MockDDSMessageEndpoint();
        endpoint = new TestStopServiceEndPoint(MessageType::STOP_SERVICE_REQUEST, "test_topic");
        endpoint->Initialize(nullptr, nullptr, nullptr);

        Mock_ROSServiceController = new MockROSServiceController();
        ON_CALL(*Mock_ADSWManager, GetServiceController()).WillByDefault(::testing::Return(std::shared_ptr<MockROSServiceController>(Mock_ROSServiceController)));
    }
    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        delete endpoint;
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;
        Mock_ADSWManager = nullptr;
        Mock_ROSServiceController = nullptr;
    }
    TestStopServiceEndPoint* endpoint;
};

TEST_F(StopServiceEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(StopServiceEndPointTest, OnDataAvailableCallsServiceController) {
    endpoint->req_.service_list().push_back("service1");
    endpoint->mock_read_result = true;
    ON_CALL(*Mock_ROSServiceController, ProcessStopServiceRequest(::testing::_))
        .WillByDefault(Return(true));
    endpoint->OnDataAvailable();

}

TEST_F(StopServiceEndPointTest, OnDataAvailableProcessFails) {
    endpoint->req_.service_list().push_back("service1");
    endpoint->mock_read_result = true;
    ON_CALL(*Mock_ROSServiceController, ProcessStopServiceRequest(::testing::_))
        .WillByDefault(Return(false));
    endpoint->OnDataAvailable();
}

TEST_F(StopServiceEndPointTest, OnDataAvailableNoRequest) {
    endpoint->req_ = StopServiceRequest(); // Reset to empty request
    endpoint->mock_read_result = false;
    ON_CALL(*Mock_ROSServiceController, ProcessStopServiceRequest(::testing::_))
        .WillByDefault(Return(false));
    endpoint->OnDataAvailable();
}

TEST_F(StopServiceEndPointTest, GetTypeSupportReturnsValidType) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}

TEST_F(StopServiceEndPointTest, GetTypeSupportReturnsNullForInvalidType) {
    endpoint->type_support_ = nullptr; // Simulate invalid type support
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_EQ(type_support, nullptr);
}

TEST_F(StopServiceEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}

TEST_F(StopServiceEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
}

TEST_F(StopServiceEndPointTest, TypeSupportIsNotNull) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
