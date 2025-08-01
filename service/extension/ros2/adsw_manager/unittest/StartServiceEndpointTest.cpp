// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/StartServiceEndPoint.hpp"

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

// Mock ROSServiceController
class MockROSServiceController : public ROSServiceController {
public:
    bool ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message) override {
        called = true;
        last_size = message ? message->service_list().size() : -1;
        return process_result;
    }
    bool called = false;
    int last_size = -1;
    bool process_result = true;
};

// Mock ADSWManager
class MockADSWManager : public ADSWManager {
public:
    static MockROSServiceController* controller;
    std::shared_ptr<ROSServiceController> GetServiceController() override { return std::shared_ptr<ROSServiceController>(controller); }
};
MockROSServiceController* MockADSWManager::controller = nullptr;

class StartServiceEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockADSWManager::controller = new MockROSServiceController();
        endpoint = new ros2::adsw_manager::StartServiceEndPoint(MessageType::START_SERVICE_REQUEST, "test_topic");
        endpoint->type_support_ = std::make_shared<MockTypeSupport>();
    }
    void TearDown() override {
        delete endpoint;
        delete MockADSWManager::controller;
    }
    ros2::adsw_manager::StartServiceEndPoint* endpoint;
};

TEST_F(StartServiceEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(StartServiceEndPointTest, OnDataAvailableCallsServiceController) {
    // Simulate Read<StartServiceRequest>(req_) == true
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::controller->process_result = true;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::controller->called);
    EXPECT_EQ(MockADSWManager::controller->last_size, 1);
}

TEST_F(StartServiceEndPointTest, OnDataAvailableProcessFails) {
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::controller->process_result = false;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::controller->called);
    EXPECT_EQ(MockADSWManager::controller->last_size, 1);
}
TEST_F(StartServiceEndPointTest, OnDataAvailableNoRequest) {
    // Simulate Read<StartServiceRequest>(req_) == false
    endpoint->req_ = StartServiceRequest(); // Reset to empty request
    MockADSWManager::controller->called = false; // Reset call status
    endpoint->OnDataAvailable();
    EXPECT_FALSE(MockADSWManager::controller->called);
}
TEST_F(StartServiceEndPointTest, GetTypeSupportReturnsValidType) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}
TEST_F(StartServiceEndPointTest, GetTypeSupportReturnsNullForInvalidType) {
    endpoint->type_support_ = nullptr; // Simulate invalid type support
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_EQ(type_support, nullptr);
}
TEST_F(StartServiceEndPointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(StartServiceEndPointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
}
TEST_F(StartServiceEndPointTest, TypeSupportIsNotNull) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
    EXPECT_EQ(type_support->get_type_name(), "MockType");
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
