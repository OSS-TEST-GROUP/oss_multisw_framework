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
        ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
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
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_,_)).WillOnce(Return(true));
 
    EXPECT_TRUE(ins->StartService(mock_service));

    free(fake_buffer);
}

// Test StopService sends correct message
TEST_F(StateManagerTest, StopService_SendsMessage) {

    void* fake_buffer = malloc(sizeof(StopServiceRequest));

    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
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
    ON_CALL(*mock_service1, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
    ON_CALL(*Mock_ManagerBase, GetService(_)).WillByDefault(Return(mock_service1));

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetService(_)).WillOnce(Return(mock_service1));
    EXPECT_CALL(*mock_transmitter, SendMessage(_,_)).WillOnce(Return(true));
 
    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test ProcessServiceHandling calls PolicyManager and acts accordingly
TEST_F(StateManagerTest, ProcessServiceHandling_StartAndStop) {
    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service2 = std::make_shared<MockService>();
    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service1, mock_service2};

    // Set up mock behaviors for both services
    ON_CALL(*mock_service1, GetName()).WillByDefault(Return("TestService1"));
    ON_CALL(*mock_service1, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
    ON_CALL(*mock_service2, GetName()).WillByDefault(Return("TestService2"));
    ON_CALL(*mock_service2, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));

    EXPECT_CALL(*Mock_ManagerBase, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillRepeatedly(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillRepeatedly(Return(true));
    
    // Use WillRepeatedly for multiple calls with different return values based on argument
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_))
        .WillRepeatedly(Return(lifecycle_manager::ServiceControlAction::START));

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
    Mock_Service = mock_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("TestService"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_FALSE(ins->StartService(mock_service));
}

TEST_F(StateManagerTest, StopService_NullTransmitter) {

    auto mock_service = std::make_shared<MockService>();
    Mock_Service = mock_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("TestService"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(ins->StopService(mock_service));
}

TEST_F(StateManagerTest, SetServiceMode_NullService) {

    SetServiceModeRequest req;
    req.service_list().push_back("non_existent_svc");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    EXPECT_CALL(*Mock_ManagerBase, GetService("non_existent_svc")).WillOnce(Return(nullptr));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));

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

    EXPECT_CALL(*Mock_ManagerBase, GetService("svc1")).WillOnce(Return(mock_service1));
    EXPECT_CALL(*Mock_ManagerBase, GetService("svc2")).WillOnce(Return(mock_service2));
    EXPECT_CALL(*Mock_ManagerBase, GetService("svc3")).WillOnce(Return(mock_service3));

    // SetServiceMode on Service class forwards to Mock_Service
    // We need to set Mock_Service for each call, but since they happen in sequence,
    // we can use ON_CALL to allow any calls
    ON_CALL(*mock_service1, SetServiceMode(_)).WillByDefault(Return());
    ON_CALL(*mock_service2, SetServiceMode(_)).WillByDefault(Return());
    ON_CALL(*mock_service3, SetServiceMode(_)).WillByDefault(Return());
    
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NoAction) {

    auto mock_service = std::make_shared<MockService>();
    Mock_Service = mock_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("TestService"));

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    // PolicyManager::Evaluate() is non-virtual — calls forward through Mock_PolicyManager global
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillOnce(Return(lifecycle_manager::ServiceControlAction::NONE));

    ins->ProcessServiceHandling(services);
}

TEST_F(StateManagerTest, ProcessServiceHandling_EmptyServiceList) {

    std::set<std::shared_ptr<lifecycle_manager::Service>> services;

    EXPECT_NO_THROW(ins->ProcessServiceHandling(services));
}

TEST_F(StateManagerTest, ProcessServiceHandling_NullTransmitter) {
    StateManager* state_mgr = new StateManager(*Mock_LifecycleManager);

    auto mock_service = std::make_shared<MockService>();
    Mock_Service = mock_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("TestService"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));

    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service};

    // PolicyManager::Evaluate() is non-virtual — calls forward through Mock_PolicyManager global
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillOnce(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr->ProcessServiceHandling(services));

    delete state_mgr;
}

TEST_F(StateManagerTest, RequestStopAll_WithServices) {
    StateManager state_mgr(*Mock_LifecycleManager);

    auto mock_transmitter_ptr = std::make_shared<MockDDSTransmitter>();
    auto mock_registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = mock_registry.get();  // Set global pointer for forwarding

    std::vector<std::string> groups = {"group1", "group2"};
    std::vector<std::string> group1_services = {"svc1", "svc2"};
    std::vector<std::string> group2_services = {"svc3"};

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillRepeatedly(Return(mock_transmitter_ptr));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry()).WillOnce(Return(mock_registry));
    EXPECT_CALL(*mock_registry, GetGroupList()).WillOnce(Return(groups));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group1")).WillOnce(Return(group1_services));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("group2")).WillOnce(Return(group2_services));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
    Mock_ServiceRegistry = nullptr;  // Reset global pointer
}

TEST_F(StateManagerTest, RequestStopAll_NullTransmitter) {
    StateManager state_mgr(*Mock_LifecycleManager);

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

TEST_F(StateManagerTest, RequestStopAll_NullRegistry) {
    StateManager state_mgr(*Mock_LifecycleManager);

    auto mock_transmitter_ptr = std::make_shared<MockDDSTransmitter>();

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter_ptr));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(state_mgr.RequestStopAll());
}

// Additional tests for branch coverage

// Test StartService with different MwTypes
TEST_F(StateManagerTest, StartService_AGLMwType) {
    ON_CALL(*mock_service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::AGL));
    
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("AGLService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::AGL));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    
    EXPECT_TRUE(ins->StartService(mock_service));
}

TEST_F(StateManagerTest, StartService_AUTOSARMwType) {
    ON_CALL(*mock_service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::AUTOSAR));
    
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("AUTOSARService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::AUTOSAR));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    
    EXPECT_TRUE(ins->StartService(mock_service));
}

// Test StartService when SendMessage returns false
TEST_F(StateManagerTest, StartService_SendMessageFails) {
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("TestService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(false));
    
    EXPECT_FALSE(ins->StartService(mock_service));
}

// Test StopService with different MwTypes
TEST_F(StateManagerTest, StopService_AGLMwType) {
    ON_CALL(*mock_service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::AGL));
    
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("AGLService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::AGL));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    
    EXPECT_NO_THROW(ins->StopService(mock_service));
}

// Test SetServiceMode with null transmitter (response not sent)
TEST_F(StateManagerTest, SetServiceMode_NullTransmitter) {
    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    auto mock_service1 = std::make_shared<MockService>();
    Mock_Service = mock_service1.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));

    EXPECT_CALL(*Mock_ManagerBase, GetService(_)).WillOnce(Return(mock_service1));
    // SetServiceMode call is forwarded to Mock_Service
    EXPECT_CALL(*Mock_Service, SetServiceMode(lifecycle_manager::ServiceMode::NORMAL)).Times(1);
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(nullptr));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test SetServiceMode with OTA_MODE
TEST_F(StateManagerTest, SetServiceMode_OTAMode) {
    SetServiceModeRequest req;
    req.service_list().push_back("svc1");
    req.service_mode() = lifecyclemgr::ServiceMode::OTA_MODE;

    auto mock_service1 = std::make_shared<MockService>();
    Mock_Service = mock_service1.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));

    EXPECT_CALL(*Mock_ManagerBase, GetService(_)).WillOnce(Return(mock_service1));
    EXPECT_CALL(*Mock_Service, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE)).Times(1);
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test SetServiceMode with empty service list
TEST_F(StateManagerTest, SetServiceMode_EmptyServiceList) {
    SetServiceModeRequest req;
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;
    // Empty service list

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test SetServiceMode with mixed valid and null services
TEST_F(StateManagerTest, SetServiceMode_MixedValidAndNullServices) {
    SetServiceModeRequest req;
    req.service_list().push_back("valid_svc");
    req.service_list().push_back("null_svc");
    req.service_list().push_back("another_valid");
    req.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service3 = std::make_shared<MockService>();
    
    // Since SetServiceMode forwards to Mock_Service and we have multiple services,
    // we'll use WillRepeatedly to handle any SetServiceMode calls
    Mock_Service = mock_service1.get();
    ON_CALL(*Mock_Service, SetServiceMode(_)).WillByDefault(Return());

    EXPECT_CALL(*Mock_ManagerBase, GetService("valid_svc")).WillOnce(Return(mock_service1));
    EXPECT_CALL(*Mock_ManagerBase, GetService("null_svc")).WillOnce(Return(nullptr));
    EXPECT_CALL(*Mock_ManagerBase, GetService("another_valid")).WillOnce(Return(mock_service3));
    
    // Can't verify exact calls on individual services due to global mock pointer
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(ins->SetServiceMode(&req));
}

// Test ProcessServiceHandling with only START actions
TEST_F(StateManagerTest, ProcessServiceHandling_OnlyStartActions) {
    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service2 = std::make_shared<MockService>();
    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service1, mock_service2};

    ON_CALL(*mock_service1, GetName()).WillByDefault(Return("svc1"));
    ON_CALL(*mock_service2, GetName()).WillByDefault(Return("svc2"));

    EXPECT_CALL(*Mock_ManagerBase, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillRepeatedly(Return(lifecycle_manager::ServiceControlAction::START));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_SERVICE_REQUEST, _)).WillOnce(Return(true));

    ins->ProcessServiceHandling(services);
}

// Test ProcessServiceHandling with only STOP actions
TEST_F(StateManagerTest, ProcessServiceHandling_OnlyStopActions) {
    auto mock_service1 = std::make_shared<MockService>();
    auto mock_service2 = std::make_shared<MockService>();
    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service1, mock_service2};

    ON_CALL(*mock_service1, GetName()).WillByDefault(Return("svc1"));
    ON_CALL(*mock_service2, GetName()).WillByDefault(Return("svc2"));

    EXPECT_CALL(*Mock_ManagerBase, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillRepeatedly(Return(lifecycle_manager::ServiceControlAction::STOP));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_SERVICE_REQUEST, _)).WillOnce(Return(true));

    ins->ProcessServiceHandling(services);
}

// Test ProcessServiceHandling with all NONE actions (no messages sent)
TEST_F(StateManagerTest, ProcessServiceHandling_AllNoneActions) {
    auto mock_service1 = std::make_shared<MockService>();
    std::set<std::shared_ptr<lifecycle_manager::Service>> services = {mock_service1};

    EXPECT_CALL(*Mock_ManagerBase, GetPolicy()).WillRepeatedly(Return(mock_policy));
    EXPECT_CALL(*Mock_PolicyManager, Evaluate(_)).WillOnce(Return(lifecycle_manager::ServiceControlAction::NONE));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    // No SendMessage calls expected since both lists are empty

    ins->ProcessServiceHandling(services);
}

// Test RequestStopAll with empty groups
TEST_F(StateManagerTest, RequestStopAll_EmptyGroups) {
    auto mock_registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = mock_registry.get();  // Set global pointer for forwarding
    std::vector<std::string> empty_groups;

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry()).WillOnce(Return(mock_registry));
    EXPECT_CALL(*mock_registry, GetGroupList()).WillOnce(Return(empty_groups));

    EXPECT_NO_THROW(ins->RequestStopAll());
    Mock_ServiceRegistry = nullptr;  // Reset global pointer
}

// Test RequestStopAll with group having empty service list
TEST_F(StateManagerTest, RequestStopAll_EmptyServiceListInGroup) {
    auto mock_registry = std::make_shared<MockServiceRegistry>();
    Mock_ServiceRegistry = mock_registry.get();  // Set global pointer for forwarding
    std::vector<std::string> groups = {"empty_group"};
    std::vector<std::string> empty_services;

    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillRepeatedly(Return(mock_transmitter));
    EXPECT_CALL(*Mock_ManagerBase, GetRegistry()).WillOnce(Return(mock_registry));
    EXPECT_CALL(*mock_registry, GetGroupList()).WillOnce(Return(groups));
    EXPECT_CALL(*mock_registry, GetServicesNameByGroup("empty_group")).WillOnce(Return(empty_services));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_SERVICE_REQUEST, _)).WillOnce(Return(true));

    EXPECT_NO_THROW(ins->RequestStopAll());
    Mock_ServiceRegistry = nullptr;  // Reset global pointer
}

// Test OnStart with FAILED return code
TEST_F(StateManagerTest, OnStart_FailedReturnCode) {
    StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_NO_THROW(ins->OnStart(&start_resp));
}

// Test OnStop with FAILED return code
TEST_F(StateManagerTest, OnStop_FailedReturnCode) {
    StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_NO_THROW(ins->OnStop(&stop_resp));
}

// Test multiple sequential StartService calls
TEST_F(StateManagerTest, StartService_MultipleCalls) {
    // Service::GetName()/GetMwType() are non-virtual — both calls route through Mock_Service global
    // First call
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("FirstService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    EXPECT_TRUE(ins->StartService(mock_service));

    // Second call - should clear and repopulate service_list
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("SecondService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    EXPECT_TRUE(ins->StartService(mock_service));
}

// Test multiple sequential StopService calls
TEST_F(StateManagerTest, StopService_MultipleCalls) {
    // Service::GetName()/GetMwType() are non-virtual — both calls route through Mock_Service global
    // First call
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("FirstService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    ins->StopService(mock_service);

    // Second call - should clear and repopulate service_list
    EXPECT_CALL(*mock_service, GetName()).WillOnce(Return("SecondService"));
    EXPECT_CALL(*mock_service, GetMwType()).WillOnce(Return(lifecycle_manager::MwType::ROS2));
    EXPECT_CALL(*Mock_ManagerBase, GetTransmitter()).WillOnce(Return(mock_transmitter));
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).WillOnce(Return(true));
    ins->StopService(mock_service);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
