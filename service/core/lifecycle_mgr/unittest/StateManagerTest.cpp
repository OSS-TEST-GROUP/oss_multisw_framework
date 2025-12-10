// Unit tests for StateManager class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock/mock_lifecycle_manager.hpp"
#include "mock/mock_state_manager.hpp"
#include "mock/mock_service.hpp"
#include "mock/mock_dds_transmitter.hpp"
#include "mock/mock_dds_message_endpoint.hpp"
#include "mock/mock_message_endpoint.hpp"
#include "mock/mock_policy_manager.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_mw_service_manager.hpp" 
#include "mock/mock_manager_base.hpp"   

#include "include/TypeConvert.hpp"

using namespace lifecycle_manager;

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

class StateManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }

};

TEST_F(StateManagerTest, StartAndStop) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);
    EXPECT_CALL(state_mgr, Start()).WillOnce(::testing::Return(true));
    EXPECT_TRUE(state_mgr.Start());
    EXPECT_CALL(state_mgr, Stop()).Times(1);
    state_mgr.Stop();
}


// Test StartService sends correct message
TEST_F(StateManagerTest, StartService_SendsMessage) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();
    auto mock_endpoint = std::make_shared<MockDDSMessageEndPoint>("dds_topic");

    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, GetMessageEndPoint(_)).WillOnce(Return(mock_endpoint));
    void* fake_buffer = malloc(sizeof(StartServiceRequest));
    EXPECT_CALL(*mock_endpoint, Write(fake_buffer)).Times(1);

    EXPECT_TRUE(state_mgr.StartService(mock_service));
    free(fake_buffer);
}

// Test StopService sends correct message
TEST_F(StateManagerTest, StopService_SendsMessage) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();
    auto mock_endpoint = std::make_shared<MockDDSMessageEndPoint>("dds_topic");

    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, GetMessageEndPoint(_)).WillOnce(Return(mock_endpoint));
    void* fake_buffer = malloc(sizeof(StopServiceRequest));
    EXPECT_CALL(*mock_endpoint, Write(fake_buffer)).Times(1);

    state_mgr.StopService(mock_service);
    free(fake_buffer);
}

// Test SetServiceMode sets mode on all services
TEST_F(StateManagerTest, SetServiceMode_SetsMode) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(mgr, GetService("svc1")).WillOnce(Return(mock_service));
    EXPECT_CALL(*mock_service, SetServiceMode(lifecycle_manager::ServiceMode::NORMAL)).Times(1);

    EXPECT_TRUE(state_mgr.SetServiceMode(&req));
}

// Test ProcessServiceHandling calls PolicyManager and acts accordingly
TEST_F(StateManagerTest, ProcessServiceHandling_StartAndStop) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    auto mock_policy = std::make_shared<MockPolicyManager>();
    auto mock_service1 = std::make_shared<lifecycle_manager::Service>();
    auto mock_service2 = std::make_shared<lifecycle_manager::Service>();

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service1, mock_service2};

    EXPECT_CALL(mgr, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service1)).WillOnce(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service2)).WillOnce(Return(lifecycle_manager::ServiceControlAction::STOP));
    EXPECT_CALL(state_mgr, StartService(mock_service1)).Times(1);
    EXPECT_CALL(state_mgr, StopService(mock_service2)).Times(1);

    state_mgr.ProcessServiceHandling(services);
}

// Test OnStart and OnStop handle error codes
TEST_F(StateManagerTest, OnStart_OnStop_ErrorHandling) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    state_mgr.OnStart(&start_resp);

    StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    state_mgr.OnStop(&stop_resp);

    // No assertions, just ensure no crash
}

TEST_F(StateManagerTest, OnStart_SuccessCode) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_NO_THROW(state_mgr.OnStart(&start_resp));
}

TEST_F(StateManagerTest, OnStop_SuccessCode) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_NO_THROW(state_mgr.OnStop(&stop_resp));
}

TEST_F(StateManagerTest, OnStart_NullMessage) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    EXPECT_NO_THROW(state_mgr.OnStart(nullptr));
}

TEST_F(StateManagerTest, OnStop_NullMessage) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    EXPECT_NO_THROW(state_mgr.OnStop(nullptr));
}

TEST_F(StateManagerTest, StartService_NullTransmitter) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_FALSE(state_mgr.StartService(mock_service));
}

TEST_F(StateManagerTest, StopService_NullTransmitter) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.StopService(mock_service));
}

TEST_F(StateManagerTest, SetServiceMode_NullService) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    SetServiceModeRequest req;
    req.service_list().push_back("non_existent_svc");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    EXPECT_CALL(mgr, GetService("non_existent_svc")).WillOnce(Return(nullptr));

    EXPECT_TRUE(state_mgr.SetServiceMode(&req));
}

TEST_F(StateManagerTest, SetServiceMode_MultipleServices) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_list().push_back("svc2");
    req.service_list().push_back("svc3");
    req.service_mode() = lifecyclemgr::ServiceMode::OTA_MODE;

    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service2 = std::make_shared<MockService>();
    auto mock_service3 = std::make_shared<MockService>();

    EXPECT_CALL(mgr, GetService("svc1")).WillOnce(Return(mock_service1));
    EXPECT_CALL(mgr, GetService("svc2")).WillOnce(Return(mock_service2));
    EXPECT_CALL(mgr, GetService("svc3")).WillOnce(Return(mock_service3));

    EXPECT_CALL(*mock_service1, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);
    EXPECT_CALL(*mock_service2, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);
    EXPECT_CALL(*mock_service3, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);

    EXPECT_TRUE(state_mgr.SetServiceMode(&req));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NoAction) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    auto mock_policy = std::make_shared<MockPolicyManager>();
    auto mock_service = std::make_shared<lifecycle_manager::Service>();

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    EXPECT_CALL(mgr, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service)).WillOnce(Return(lifecycle_manager::ServiceControlAction::NONE));

    state_mgr.ProcessServiceHandling(services);
}

TEST_F(StateManagerTest, ProcessServiceHandling_EmptyServiceList) {
    MockLifecycleManager mgr;
    MockStateManager state_mgr(mgr);

    std::set<std::shared_ptr<lifecycle_manager::Service>> services;

    EXPECT_NO_THROW(state_mgr.ProcessServiceHandling(services));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NullTransmitter) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    auto mock_policy = std::make_shared<MockPolicyManager>();
    auto mock_service = std::make_shared<lifecycle_manager::Service>();

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    EXPECT_CALL(mgr, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service)).WillOnce(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.ProcessServiceHandling(services));
}

TEST_F(StateManagerTest, RequestStopAll_WithServices) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();
    auto mock_registry = std::make_shared<MockServiceRegistry>();

    std::vector<std::string> groups = {"group1", "group2"};
    std::vector<std::string> group1_services = {"svc1", "svc2"};
    std::vector<std::string> group2_services = {"svc3"};

    EXPECT_CALL(mgr, GetTransmitter()).WillRepeatedly(Return(mock_transmitter));
    EXPECT_CALL(mgr, GetRegistry()).WillOnce(Return(mock_registry));
    EXPECT_CALL(*mock_registry, GetGroupList()).WillOnce(Return(groups));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group1")).WillOnce(Return(group1_services));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group2")).WillOnce(Return(group2_services));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

TEST_F(StateManagerTest, RequestStopAll_NullTransmitter) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

TEST_F(StateManagerTest, RequestStopAll_NullRegistry) {
    MockLifecycleManager mgr;
    StateManager state_mgr(mgr);

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();

    EXPECT_CALL(mgr, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(mgr, GetRegistry()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
