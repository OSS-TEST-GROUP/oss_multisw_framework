// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/endpoint/GetServiceStateEndPoint.hpp"


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

// Mock ServiceRegistry
class MockServiceRegistry : public ServiceRegistry {
public:
    bool ProcessGetServiceStateRequest(GetServiceStateRequest* message) override {
        called = true;
        last_size = message ? message->service_list().size() : -1;
        return process_result;
    }
    bool called = false;
    int last_size = -1;
    bool process_result = true;
};


class GetServiceStateEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockADSWManager::registry = new MockServiceRegistry();
        endpoint = new ros2::adsw_manager::GetServiceStateEndPoint(MessageType::GET_SERVICE_STATE_REQUEST, "test_topic");
        endpoint->type_support_ = std::make_shared<MockTypeSupport>();
    }
    void TearDown() override {
        delete endpoint;
        delete MockADSWManager::registry;
    }
    ros2::adsw_manager::GetServiceStateEndPoint* endpoint;
};

TEST_F(GetServiceStateEndPointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "MockType");
    EXPECT_EQ(endpoint->GetTypeSupport()->get_type_name(), "MockType");
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailableCallsServiceRegistry) {
    // Simulate Read<GetServiceStateRequest>(req_) == true
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::registry->process_result = true;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::registry->called);
    EXPECT_EQ(MockADSWManager::registry->last_size, 1);
}

TEST_F(GetServiceStateEndPointTest, OnDataAvailableProcessFails) {
    endpoint->req_.service_list().push_back("service1");
    MockADSWManager::registry->process_result = false;
    endpoint->OnDataAvailable();
    EXPECT_TRUE(MockADSWManager::registry->called);
    EXPECT_EQ(MockADSWManager::registry->last_size, 1);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
