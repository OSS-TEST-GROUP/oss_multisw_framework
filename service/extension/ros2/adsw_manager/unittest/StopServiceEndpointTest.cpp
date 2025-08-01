// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/StopServiceEndPoint.hpp"


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
    bool ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message) override {
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

class StopServiceEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockADSWManager::controller = new MockROSServiceController();
        endpoint = new ros2::adsw_manager::StopServiceEndPoint(MessageType::STOP_SERVICE_REQUEST, "test_topic");
        endpoint->type_support_ = std::make_shared<MockTypeSupport>();
    }
    void TearDown() override {
        delete endpoint;
        delete MockADSWManager::controller;
    }
    ros2::adsw_manager::StopServiceEndPoint* endpoint;
};

TEST_F(StopServiceEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(StopServiceEndPointTest, OnDataAvailableCallsServiceController) {
    // Simulate Read<StopServiceRequest>(req_) == true
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::controller->process_result = true;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::controller->called);
    EXPECT_EQ(MockADSWManager::controller->last_size, 1);
}

TEST_F(StopServiceEndPointTest, OnDataAvailableProcessFails) {
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::controller->process_result = false;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::controller->called);
    EXPECT_EQ(MockADSWManager::controller->last_size, 1);
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
