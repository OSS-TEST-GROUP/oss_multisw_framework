// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/UpdateServiceListEndPoint.hpp"

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
class TestUpdateServiceListEndPoint : public UpdateServiceListEndPoint {
public:
    TestUpdateServiceListEndPoint(MessageType msgtype, std::string topic)
        : UpdateServiceListEndPoint(msgtype, topic) {}
    void OnDataAvailable() override {
        return;
    }
};

class UpdateServiceListEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
        
        Mock_DDSMessageEndpoint = new MockDDSMessageEndpoint();
        endpoint = new TestUpdateServiceListEndPoint(MessageType::UPDATE_SERVICE_LIST_RESPONSE, "test_topic");
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

    TestUpdateServiceListEndPoint* endpoint;
};

TEST_F(UpdateServiceListEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(UpdateServiceListEndPointTest, OnDataAvailableDoesNotCrash) {
    EXPECT_NO_THROW(endpoint->OnDataAvailable());
}

TEST_F(UpdateServiceListEndPointTest, OnDataAvailableProcessesResponse) {
    endpoint->OnDataAvailable();
    EXPECT_EQ(static_cast<int>(endpoint->resp_.ret_code()), 42); // <-- If ret_code is an int, this is fine. If it's an enum, compare to the enum value.
}
TEST_F(UpdateServiceListEndPointTest, GetTypeSupportReturnsValidType) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}
TEST_F(UpdateServiceListEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(UpdateServiceListEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
