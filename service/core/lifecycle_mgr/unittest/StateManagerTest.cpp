// Unit tests for StateManager class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "state/StateManager.hpp"

#include "mock/mock_lifecycle_manager.hpp"
#include "mock/mock_service.hpp"
#include "mock/mock_dds_transmitter.hpp"
#include "mock/mock_dds_message_endpoint.hpp"
#include "mock/mock_message_endpoint.hpp"
#include "mock/mock_policy_manager.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_mw_service_manager.hpp" 
#include "mock/mock_manager_base.hpp"   
#include "mock/mock_logger.hpp"

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
        Mock_LifecycleManager = new MockLifecycleManager();
        ins = new StateManager(*Mock_LifecycleManager);
        Mock_Logger = new MockLogger();
        Mock_ManagerBase = new MockManagerBase(*Mock_LifecycleManager);
        Mock_PolicyManager = new MockPolicyManager();
        Mock_DDSTransmitter = new MockDDSTransmitter();
        Mock_Service = new MockService();

        mock_endpoint = std::make_shared<MockDDSMessageEndPoint>("dds_topic");
        mock_transmitter = std::shared_ptr<MockDDSTransmitter>(Mock_DDSTransmitter);
        mock_service = std::shared_ptr<MockService>(Mock_Service);
        mock_policy = std::shared_ptr<MockPolicyManager>(Mock_PolicyManager);

        defaultValues();
    }

    void TearDown() override {
        delete ins;
        delete Mock_Logger;
        delete Mock_ManagerBase;
        delete Mock_LifecycleManager;


        mock_endpoint.reset();
        mock_transmitter.reset();
        mock_service.reset();
        mock_policy.reset();

        Mock_Logger = nullptr;
        Mock_ManagerBase = nullptr;
        Mock_PolicyManager = nullptr;
        Mock_LifecycleManager = nullptr;

        Mock_DDSTransmitter = nullptr;
        Mock_Service = nullptr;
    }

    void defaultValues() {

        // Default mock behaviors can be set here if needed
        ON_CALL(*Mock_ManagerBase, GetTransmitter()).WillByDefault(Return(mock_transmitter));
        ON_CALL(*Mock_ManagerBase, GetPolicy()).WillByDefault(Return(mock_policy));

        ON_CALL(*mock_transmitter, GetMessageEndPoint(_)).WillByDefault(Return(mock_endpoint));
        ON_CALL(*mock_transmitter, SendMessage(_, _)).WillByDefault(Return(true));
        ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("TestService"));
        ON_CALL(*Mock_Service, GetOsType()).WillByDefault(Return(lifecycle_manager::OsType::ROS2));
        ON_CALL(*mock_endpoint, Write(_)).WillByDefault(Return(true));
    }

    StateManager* ins;
    std::shared_ptr<MockDDSMessageEndPoint> mock_endpoint;
    std::shared_ptr<MockDDSTransmitter> mock_transmitter;
    std::shared_ptr<MockService> mock_service;
    std::shared_ptr<MockPolicyManager> mock_policy;
};

TEST_F(StateManagerTest, StartAndStop) {
    EXPECT_TRUE(ins->Start());
    ins->Stop();
}


// Test StartService sends correct message
TEST_F(StateManagerTest, StartService_SendsMessage) {

    void* fake_buffer = malloc(sizeof(StartServiceRequest));

    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));
    EXPECT_CALL(*mock_service, GetOsType()).WillOnce(Return(lifecycle_manager::OsType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_,_)).WillOnce(Return(true));
 
    EXPECT_TRUE(ins->StartService(mock_service));

    free(fake_buffer);
}

// Test StopService sends correct message
TEST_F(StateManagerTest, StopService_SendsMessage) {

    void* fake_buffer = malloc(sizeof(StopServiceRequest));

    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));
    EXPECT_CALL(*mock_service, GetOsType()).WillOnce(Return(lifecycle_manager::OsType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_,_)).WillOnce(Return(true));
 
    ins->StopService(std::shared_ptr<lifecycle_manager::Service>(mock_service));

    free(fake_buffer);
}

// Test SetServiceMode sets mode on all services
TEST_F(StateManagerTest, SetServiceMode_SetsMode) {

    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    auto mock_service1 = std::make_shared<MockService>();

    ON_CALL(*mock_service1, GetName()).WillByDefault(Return("svc1"));
    ON_CALL(*mock_service1, GetOsType()).WillByDefault(Return(lifecycle_manager::OsType::ROS2));
    ON_CALL(*Mock_ManagerBase, GetService(_)).WillByDefault(Return(mock_service1));

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetService(_)).WillOnce(Return(mock_service1));
    EXPECT_CALL(*mock_transmitter, SendMessage(_,_)).WillOnce(Return(true));
 
    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test ProcessServiceHandling calls PolicyManager and acts accordingly
TEST_F(StateManagerTest, ProcessServiceHandling_StartAndStop) {
    auto mock_service1 = std::make_shared<MockService>();
    std::shared_ptr<lifecycle_manager::Service> service1 = mock_service1;

    auto mock_service2 = std::make_shared<MockService>();
    std::shared_ptr<lifecycle_manager::Service> service2 = mock_service2;
    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {service1, service2};

    ON_CALL(*Mock_PolicyManager, Evaluate(service1)).WillByDefault(Return(lifecycle_manager::ServiceControlAction::START));
    ON_CALL(*Mock_PolicyManager, Evaluate(service2)).WillByDefault(Return(lifecycle_manager::ServiceControlAction::STOP));

    EXPECT_CALL(*Mock_ManagerBase, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillOnce(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillOnce(Return(lifecycle_manager::ServiceControlAction::STOP));

    ins->ProcessServiceHandling(services);
}

// Test OnStart and OnStop handle error codes
TEST_F(StateManagerTest, OnStart_OnStop_ErrorHandling) {

    StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    ins->OnStart(&start_resp);

    StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    ins->OnStop(&stop_resp);

    // No assertions, just ensure no crash
}

TEST_F(StateManagerTest, OnStart_SuccessCode) {

    StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_NO_THROW(ins->OnStart(&start_resp));
}

TEST_F(StateManagerTest, OnStop_SuccessCode) {

    StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_NO_THROW(ins->OnStop(&stop_resp));
}

TEST_F(StateManagerTest, OnStart_NullMessage) {

    EXPECT_NO_THROW(ins->OnStart(nullptr));
}

TEST_F(StateManagerTest, OnStop_NullMessage) {

    EXPECT_NO_THROW(ins->OnStop(nullptr));
}

TEST_F(StateManagerTest, StartService_NullTransmitter) {

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_FALSE(ins->StartService(mock_service));
}

TEST_F(StateManagerTest, StopService_NullTransmitter) {

    auto mock_service = std::make_shared<MockService>();
    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(ins->StopService(mock_service));
}

TEST_F(StateManagerTest, SetServiceMode_NullService) {

    SetServiceModeRequest req;
    req.service_list().push_back("non_existent_svc");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    EXPECT_CALL(*Mock_LifecycleManager, GetService("non_existent_svc")).WillOnce(Return(nullptr));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

TEST_F(StateManagerTest, SetServiceMode_MultipleServices) {

    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_list().push_back("svc2");
    req.service_list().push_back("svc3");
    req.service_mode() = lifecyclemgr::ServiceMode::OTA_MODE;

    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service2 = std::make_shared<MockService>();
    auto mock_service3 = std::make_shared<MockService>();

    EXPECT_CALL(*Mock_LifecycleManager, GetService("svc1")).WillOnce(Return(mock_service1));
    EXPECT_CALL(*Mock_LifecycleManager, GetService("svc2")).WillOnce(Return(mock_service2));
    EXPECT_CALL(*Mock_LifecycleManager, GetService("svc3")).WillOnce(Return(mock_service3));

    EXPECT_CALL(*mock_service1, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);
    EXPECT_CALL(*mock_service2, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);
    EXPECT_CALL(*mock_service3, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NoAction) {

    auto mock_policy = std::make_shared<MockPolicyManager>();
    auto mock_service = std::make_shared<lifecycle_manager::Service>();

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    EXPECT_CALL(*Mock_LifecycleManager, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service)).WillOnce(Return(lifecycle_manager::ServiceControlAction::NONE));

    ins->ProcessServiceHandling(services);
}

TEST_F(StateManagerTest, ProcessServiceHandling_EmptyServiceList) {

    std::set<std::shared_ptr<lifecycle_manager::Service>> services;

    EXPECT_NO_THROW(ins->ProcessServiceHandling(services));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NullTransmitter) {
    StateManager* state_mgr = new StateManager(*Mock_LifecycleManager);

    auto mock_policy = std::make_shared<MockPolicyManager>();
    auto mock_service = std::make_shared<lifecycle_manager::Service>();

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    EXPECT_CALL(*Mock_LifecycleManager, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*mock_policy, Evaluate(mock_service)).WillOnce(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr->ProcessServiceHandling(services));
}

TEST_F(StateManagerTest, RequestStopAll_WithServices) {
    StateManager state_mgr(*Mock_LifecycleManager);

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();
    auto mock_registry = std::make_shared<MockServiceRegistry>();

    std::vector<std::string> groups = {"group1", "group2"};
    std::vector<std::string> group1_services = {"svc1", "svc2"};
    std::vector<std::string> group2_services = {"svc3"};

    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillRepeatedly(Return(mock_transmitter));
    EXPECT_CALL(*Mock_LifecycleManager, GetRegistry()).WillOnce(Return(mock_registry));
    EXPECT_CALL(*mock_registry, GetGroupList()).WillOnce(Return(groups));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group1")).WillOnce(Return(group1_services));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group2")).WillOnce(Return(group2_services));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

TEST_F(StateManagerTest, RequestStopAll_NullTransmitter) {
    StateManager state_mgr(*Mock_LifecycleManager);

    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

TEST_F(StateManagerTest, RequestStopAll_NullRegistry) {
    StateManager state_mgr(*Mock_LifecycleManager);

    auto mock_transmitter = std::make_shared<MockDDSTransmitter>();

    EXPECT_CALL(*Mock_LifecycleManager, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*Mock_LifecycleManager, GetRegistry()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
