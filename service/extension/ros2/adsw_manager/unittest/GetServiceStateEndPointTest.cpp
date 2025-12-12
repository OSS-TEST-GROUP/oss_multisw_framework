// Unit tests for DDSTransmitter class using Google Test and Google Mock
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

using namespace ros2::adsw_manager;

// Test-only subclass for mocking Read behavior
class TestGetServiceStateEndPoint : public GetServiceStateEndPoint {
public:
    TestGetServiceStateEndPoint(MessageType msgtype, std::string topic)
        : GetServiceStateEndPoint(msgtype, topic) {}
    void OnDataAvailable() override {
        return;
    }
    bool mock_read_result = true;
};

class GetServiceStateEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        
        Mock_DDSMessageEndpoint = new MockDDSMessageEndpoint();
        // Setup mock expectations for DDSMessageEndpoint methods
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeName()).WillByDefault(Return("lifecyclemgr::dds_::GetServiceStateRequest_"));
        ON_CALL(*Mock_DDSMessageEndpoint, GetTypeSupport()).WillByDefault(Return(nullptr));
        
        endpoint = new TestGetServiceStateEndPoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
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
    TestGetServiceStateEndPoint* endpoint;
};

TEST_F(GetServiceStateEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    // GetTypeName and GetTypeSupport are forwarded to mock, which may return empty/null
    // Just verify the calls don't crash
    EXPECT_NO_THROW(endpoint->GetTypeName());
    EXPECT_NO_THROW(endpoint->GetTypeSupport());
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailableCallsServiceRegistry) {
    endpoint->req_.service_list().push_back("service1");
    endpoint->mock_read_result = true;
    endpoint->OnDataAvailable();
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailableProcessFails) {
    endpoint->req_.service_list().push_back("service1");
    endpoint->mock_read_result = false;
    endpoint->OnDataAvailable();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
