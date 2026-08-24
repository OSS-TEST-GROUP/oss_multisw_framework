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

// ═══════════════════════════════════════════════════════════════════════════
// F013/F014 ctrl/* 게이트 핸들러
//
// StateManager 미커버분(296줄 중 186줄)이 거의 전부 여기다. 위 테스트들은
// Start/Stop/SetServiceMode/ProcessServiceHandling 만 다루고, monitor 가
// 들어오는 ctrl/* 경로(9개 메서드)는 하나도 건드리지 않았다.
//
// 게이트 로직의 요지는 "무엇을 거르고, 무엇을 어느 토픽으로 넘기고,
// 어떤 ret_code 를 돌려주는가" 세 가지다. 그 세 가지를 검증한다.
// ═══════════════════════════════════════════════════════════════════════════

class StateManagerCtrlTest : public StateManagerTest {
  protected:
    void SetUp() override {
        StateManagerTest::SetUp();
        Mock_ServiceRegistry = new ::testing::NiceMock<MockServiceRegistry>();
        mock_registry = std::shared_ptr<MockServiceRegistry>(
            Mock_ServiceRegistry, [](MockServiceRegistry*) {});
        ON_CALL(*Mock_ManagerBase, GetRegistry()).WillByDefault(Return(mock_registry));
    }
    void TearDown() override {
        mock_registry.reset();
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
        StateManagerTest::TearDown();
    }

    // 이름이 알려진 서비스 하나를 등록해 둔다.
    //
    // 주의: mock_service.hpp 는 gmock 서브클래스가 아니라 링크 심이라
    // `Service::GetServiceMode()` 등이 인스턴스와 무관하게 전역 Mock_Service 로
    // 위임된다. 그래서 테스트마다 새 MockService 를 만들어 프로그래밍하면
    // 아무 효과가 없다 — 반드시 전역 Mock_Service 를 세팅해야 한다.
    std::shared_ptr<MockService> KnownService(
            lifecycle_manager::ServiceMode mode = lifecycle_manager::ServiceMode::NORMAL,
            lifecycle_manager::MwType mw = lifecycle_manager::MwType::ROS2) {
        ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(mw));
        ON_CALL(*Mock_Service, GetServiceMode()).WillByDefault(Return(mode));
        ON_CALL(*Mock_ManagerBase, GetService(_)).WillByDefault(Return(mock_service));
        return mock_service;
    }

    void NoSuchService() {
        ON_CALL(*Mock_ManagerBase, GetService(_))
            .WillByDefault(Return(std::shared_ptr<lifecycle_manager::Service>()));
    }

    std::shared_ptr<lifecycle_manager::Container> KnownContainer() {
        auto c = std::make_shared<lifecycle_manager::Container>(
            "cnt", "img", "/tmp/c.yaml", "ns", lifecycle_manager::ContainerType::DOCKER);
        ON_CALL(*Mock_ServiceRegistry, GetContainer(_)).WillByDefault(Return(c));
        return c;
    }

    void NoSuchContainer() {
        ON_CALL(*Mock_ServiceRegistry, GetContainer(_))
            .WillByDefault(Return(std::shared_ptr<lifecycle_manager::Container>()));
    }

    void NoTransmitter() {
        ON_CALL(*Mock_ManagerBase, GetTransmitter())
            .WillByDefault(Return(std::shared_ptr<DDSTransmitter>()));
    }

    std::shared_ptr<MockServiceRegistry> mock_registry;
};

// ── null 메시지는 전부 무시 ─────────────────────────────────────────────────

TEST_F(StateManagerCtrlTest, NullMessagesAreIgnored) {
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).Times(0);
    ins->HandleCtrlStartService(nullptr);
    ins->HandleCtrlStopService(nullptr);
    ins->HandleCtrlSetServiceMode(nullptr);
    ins->HandleCtrlStartContainer(nullptr);
    ins->HandleCtrlStopContainer(nullptr);
    ins->HandleCtrlSetRecoveryPolicy(nullptr);
    ins->OnStartContainer(nullptr);
    ins->OnStopContainer(nullptr);
    ins->OnSetRecoveryPolicy(nullptr);
}

// ── 빈 목록은 FAILED 응답 한 건 ─────────────────────────────────────────────

TEST_F(StateManagerCtrlTest, EmptyServiceListPublishesFailed) {
    lifecyclemgr::CtrlStartServiceRequest req;
    // 요청 하나당 ctrl 응답 딱 한 건. 액추에이터로는 아무것도 안 나간다.
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStartService(&req);
}

TEST_F(StateManagerCtrlTest, EmptyStopListPublishesFailed) {
    lifecyclemgr::CtrlStopServiceRequest req;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStopService(&req);
}

TEST_F(StateManagerCtrlTest, EmptyModeListPublishesFailed) {
    lifecyclemgr::CtrlSetServiceModeRequest req;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlSetServiceMode(&req);
}

TEST_F(StateManagerCtrlTest, EmptyContainerListPublishesFailed) {
    lifecyclemgr::CtrlStartContainerRequest start;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStartContainer(&start);

    lifecyclemgr::CtrlStopContainerRequest stop;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStopContainer(&stop);
}

TEST_F(StateManagerCtrlTest, EmptyRecoveryListPublishesFailed) {
    lifecyclemgr::CtrlSetRecoveryPolicyRequest req;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlSetRecoveryPolicy(&req);
}

// ── 모르는 이름은 거른다 ────────────────────────────────────────────────────

TEST_F(StateManagerCtrlTest, UnknownServiceRejectedOnStart) {
    NoSuchService();
    lifecyclemgr::CtrlStartServiceRequest req;
    req.service_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_SERVICE_REQUEST, _)).Times(0);
    ins->HandleCtrlStartService(&req);
}

TEST_F(StateManagerCtrlTest, UnknownServiceRejectedOnStop) {
    NoSuchService();
    lifecyclemgr::CtrlStopServiceRequest req;
    req.service_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_SERVICE_REQUEST, _)).Times(0);
    ins->HandleCtrlStopService(&req);
}

TEST_F(StateManagerCtrlTest, UnknownServiceRejectedOnSetMode) {
    NoSuchService();
    lifecyclemgr::CtrlSetServiceModeRequest req;
    req.service_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::SET_SERVICE_MODE_REQUST, _)).Times(0);
    ins->HandleCtrlSetServiceMode(&req);
}

TEST_F(StateManagerCtrlTest, UnknownServiceRejectedOnSetRecoveryPolicy) {
    NoSuchService();
    lifecyclemgr::CtrlSetRecoveryPolicyRequest req;
    req.service_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::SET_RECOVERY_POLICY_REQUEST, _)).Times(0);
    ins->HandleCtrlSetRecoveryPolicy(&req);
}

TEST_F(StateManagerCtrlTest, UnknownContainerRejected) {
    NoSuchContainer();
    lifecyclemgr::CtrlStartContainerRequest req;
    req.container_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_CONTAINER_REQUEST, _)).Times(0);
    ins->HandleCtrlStartContainer(&req);
}

TEST_F(StateManagerCtrlTest, UnknownContainerRejectedOnStop) {
    NoSuchContainer();
    lifecyclemgr::CtrlStopContainerRequest req;
    req.container_list().push_back("ghost");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_CONTAINER_REQUEST, _)).Times(0);
    ins->HandleCtrlStopContainer(&req);
}

// ── OTA_MODE 서비스는 시작 거부 ─────────────────────────────────────────────

TEST_F(StateManagerCtrlTest, OtaModeServiceRejectedOnStart) {
    auto svc = KnownService(lifecycle_manager::ServiceMode::OTA_MODE);
    lifecyclemgr::CtrlStartServiceRequest req;
    req.service_list().push_back("svc");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_SERVICE_REQUEST, _)).Times(0);
    ins->HandleCtrlStartService(&req);
}

TEST_F(StateManagerCtrlTest, OtaModeServiceStillAllowedToStop) {
    // Stop 은 OTA_MODE 게이트가 없다 — 정지는 언제나 허용되어야 한다.
    KnownService(lifecycle_manager::ServiceMode::OTA_MODE);
    lifecyclemgr::CtrlStopServiceRequest req;
    req.service_list().push_back("svc");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_SERVICE_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStopService(&req);
}

// ── 정상 경로: 액추에이터 토픽으로 전달 ─────────────────────────────────────

TEST_F(StateManagerCtrlTest, StartServiceForwardedToActuator) {
    KnownService();
    lifecyclemgr::CtrlStartServiceRequest req;
    req.service_list().push_back("svc");
    // 정상 경로는 즉답 ctrl 응답을 내지 않는다 — OnStart 가 실제 결과를 중계한다.
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .Times(0);
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_SERVICE_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStartService(&req);
}

TEST_F(StateManagerCtrlTest, StopServiceForwardedToActuator) {
    KnownService();
    lifecyclemgr::CtrlStopServiceRequest req;
    req.service_list().push_back("svc");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_SERVICE_RESPONSE, _))
        .Times(0);
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_SERVICE_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStopService(&req);
}

TEST_F(StateManagerCtrlTest, SetServiceModeForwardsAndMirrorsLocally) {
    auto svc = KnownService();
    lifecyclemgr::CtrlSetServiceModeRequest req;
    req.service_list().push_back("svc");
    req.service_mode() = lifecyclemgr::ServiceMode::OTA_MODE;

    // 로컬 레지스트리에도 즉시 반영해야 이후 게이트 판정이 새 모드를 본다.
    EXPECT_CALL(*svc, SetServiceMode(lifecycle_manager::ServiceMode::OTA_MODE));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::SET_SERVICE_MODE_REQUST, _))
        .WillOnce(Return(true));
    // 이쪽은 왕복이 없어 즉시 SUCCESS 를 낸다.
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE, _))
        .WillOnce(Return(true));
    ins->HandleCtrlSetServiceMode(&req);
}

TEST_F(StateManagerCtrlTest, SetRecoveryPolicyForwardsAndMirrorsLocally) {
    auto svc = KnownService();
    lifecyclemgr::CtrlSetRecoveryPolicyRequest req;
    req.service_list().push_back("svc");
    req.recovery_policy() = lifecyclemgr::RecoveryPolicy::MANUAL;

    EXPECT_CALL(*svc, SetRecoveryPolicy(lifecycle_manager::RecoveryPolicy::MANUAL));
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::SET_RECOVERY_POLICY_REQUEST, _))
        .WillOnce(Return(true));
    // 최종 응답은 OnSetRecoveryPolicy 가 낸다 — 여기서는 안 낸다.
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE, _)).Times(0);
    ins->HandleCtrlSetRecoveryPolicy(&req);
}

TEST_F(StateManagerCtrlTest, StartContainerForwardedToActuator) {
    KnownContainer();
    lifecyclemgr::CtrlStartContainerRequest req;
    req.container_list().push_back("cnt");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_CONTAINER_RESPONSE, _))
        .Times(0);
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_CONTAINER_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStartContainer(&req);
}

TEST_F(StateManagerCtrlTest, StopContainerForwardedToActuator) {
    KnownContainer();
    lifecyclemgr::CtrlStopContainerRequest req;
    req.container_list().push_back("cnt");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::STOP_CONTAINER_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStopContainer(&req);
}

// Container 값 객체 자체 — 게이트가 다루는 대상이라 여기서 같이 검증한다.
TEST_F(StateManagerCtrlTest, ContainerAccessorsAndDefaults) {
    auto c = KnownContainer();
    EXPECT_EQ(c->GetName(), "cnt");
    EXPECT_EQ(c->GetImage(), "img");
    EXPECT_EQ(c->GetDeployFile(), "/tmp/c.yaml");
    EXPECT_EQ(c->GetNamespace(), "ns");
    EXPECT_EQ(c->GetType(), lifecycle_manager::ContainerType::DOCKER);
    // 생성자가 안 건드리는 필드의 기본값
    EXPECT_EQ(c->GetState(), lifecycle_manager::ContainerState::CS_UNKNOWN);
    EXPECT_EQ(c->GetExitCode(), -1);

    c->SetImage("img2");
    c->SetDeployFile("/tmp/other.yaml");
    c->SetNamespace("ns2");
    c->SetType(lifecycle_manager::ContainerType::K3S);
    c->SetState(lifecycle_manager::ContainerState::CONTAINER_RUNNING);
    c->SetExitCode(137);

    EXPECT_EQ(c->GetImage(), "img2");
    EXPECT_EQ(c->GetDeployFile(), "/tmp/other.yaml");
    EXPECT_EQ(c->GetNamespace(), "ns2");
    EXPECT_EQ(c->GetType(), lifecycle_manager::ContainerType::K3S);
    EXPECT_EQ(c->GetState(), lifecycle_manager::ContainerState::CONTAINER_RUNNING);
    EXPECT_EQ(c->GetExitCode(), 137);
}

TEST_F(StateManagerCtrlTest, NullRegistrySkipsContainerValidation) {
    // 레지스트리가 없으면 검증을 건너뛰고 그대로 전달한다 (기존 동작).
    ON_CALL(*Mock_ManagerBase, GetRegistry())
        .WillByDefault(Return(std::shared_ptr<ServiceRegistry>()));
    lifecyclemgr::CtrlStartContainerRequest req;
    req.container_list().push_back("whatever");
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::START_CONTAINER_REQUEST, _))
        .WillOnce(Return(true));
    ins->HandleCtrlStartContainer(&req);
}

// ── 트랜스미터가 없으면 어디서든 조용히 실패 ────────────────────────────────

TEST_F(StateManagerCtrlTest, NoTransmitterIsSafeEverywhere) {
    KnownService();
    KnownContainer();
    NoTransmitter();

    lifecyclemgr::CtrlStartServiceRequest s;      s.service_list().push_back("svc");
    lifecyclemgr::CtrlStopServiceRequest  t;      t.service_list().push_back("svc");
    lifecyclemgr::CtrlSetServiceModeRequest m;    m.service_list().push_back("svc");
    lifecyclemgr::CtrlSetRecoveryPolicyRequest r; r.service_list().push_back("svc");
    lifecyclemgr::CtrlStartContainerRequest cs;   cs.container_list().push_back("cnt");
    lifecyclemgr::CtrlStopContainerRequest  ct;   ct.container_list().push_back("cnt");

    EXPECT_NO_THROW({
        ins->HandleCtrlStartService(&s);
        ins->HandleCtrlStopService(&t);
        ins->HandleCtrlSetServiceMode(&m);
        ins->HandleCtrlSetRecoveryPolicy(&r);
        ins->HandleCtrlStartContainer(&cs);
        ins->HandleCtrlStopContainer(&ct);
    });
}

// ── 액추에이터 응답 중계 ────────────────────────────────────────────────────

TEST_F(StateManagerCtrlTest, OnStartContainerRelaysSuccess) {
    lifecyclemgr::StartContainerResponse res;
    res.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    ins->OnStartContainer(&res);
}

TEST_F(StateManagerCtrlTest, OnStartContainerRelaysFailure) {
    lifecyclemgr::StartContainerResponse res;
    res.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_START_CONTAINER_RESPONSE, _))
        .WillOnce(Return(true));
    ins->OnStartContainer(&res);
}

TEST_F(StateManagerCtrlTest, OnStopContainerRelaysBothVerdicts) {
    EXPECT_CALL(*mock_transmitter, SendMessage(MessageType::CTRL_STOP_CONTAINER_RESPONSE, _))
        .Times(2).WillRepeatedly(Return(true));

    lifecyclemgr::StopContainerResponse ok;
    ok.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    ins->OnStopContainer(&ok);

    lifecyclemgr::StopContainerResponse ng;
    ng.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    ins->OnStopContainer(&ng);
}

TEST_F(StateManagerCtrlTest, OnSetRecoveryPolicyRelaysBothVerdicts) {
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE, _))
        .Times(2).WillRepeatedly(Return(true));

    lifecyclemgr::SetRecoveryPolicyResponse ok;
    ok.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    ins->OnSetRecoveryPolicy(&ok);

    lifecyclemgr::SetRecoveryPolicyResponse ng;
    ng.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    ins->OnSetRecoveryPolicy(&ng);
}

TEST_F(StateManagerCtrlTest, RelayIsSafeWithoutTransmitter) {
    NoTransmitter();
    lifecyclemgr::StartContainerResponse a; a.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    lifecyclemgr::StopContainerResponse  b; b.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    lifecyclemgr::SetRecoveryPolicyResponse c; c.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_NO_THROW({
        ins->OnStartContainer(&a);
        ins->OnStopContainer(&b);
        ins->OnSetRecoveryPolicy(&c);
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// OnStart / OnStop 의 유령 응답 가드 — 수락 쪽
//
// 위 OnStart_* 테스트들은 in-flight 카운터가 0 인 채로 부른다. 그래서 전부
// "유령이라 버림" 분기로만 들어갔고, 요청이 실제로 떠 있을 때의 본문
// (카운터 감소 → ret_code 판정 → ctrl 응답 중계)은 실행 0회였다.
//
// 가드가 지키는 것: 같은 도메인의 좀비 참가자가 StartServiceResponse(FAILED)
// 를 재생하면, 가드가 없을 때는 "Service Start Failure 1" 이 계속 찍히고
// FAILED 가 lfc_monitor 로 중계돼 멀쩡한 서비스가 고장난 것처럼 보였다.
// 카운터가 실제로 줄어드는지까지 봐야 가드가 한 번만 수락한다는 게 고정된다.
//
// in-flight 카운터는 private 이고 세터가 없다. lifecycle_mgr 유닛테스트는
// -fno-access-control 로 빌드하므로 직접 세운다 (이 파일의 다른 곳과 같은 규약).
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(StateManagerCtrlTest, OnStartAcceptsResponseWhileRequestInFlight) {
    ins->start_in_flight_.store(1);

    lifecyclemgr::StartServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));

    ins->OnStart(&resp);

    EXPECT_EQ(ins->start_in_flight_.load(), 0) << "수락했으면 하나 소진해야 한다";
}

TEST_F(StateManagerCtrlTest, OnStartRelaysFailureVerdict) {
    // 실패도 그대로 중계한다 — monitor 는 "전달됨" 이 아니라 실제 결과를 봐야 한다.
    ins->start_in_flight_.store(1);

    lifecyclemgr::StartServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));

    ins->OnStart(&resp);

    EXPECT_EQ(ins->start_in_flight_.load(), 0);
}

TEST_F(StateManagerCtrlTest, OnStartDropsPhantomAndLeavesCounterAlone) {
    ins->start_in_flight_.store(0);

    lifecyclemgr::StartServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).Times(0);

    ins->OnStart(&resp);

    EXPECT_EQ(ins->start_in_flight_.load(), 0) << "음수로 내려가면 안 된다";
}

TEST_F(StateManagerCtrlTest, OnStartAcceptsOnlyAsManyAsAreInFlight) {
    // 요청 둘이 떠 있으면 응답 둘까지만 수락하고 셋째는 유령으로 버린다.
    ins->start_in_flight_.store(2);

    lifecyclemgr::StartServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .Times(2).WillRepeatedly(Return(true));

    ins->OnStart(&resp);
    ins->OnStart(&resp);
    ins->OnStart(&resp);   // 유령 — 중계되지 않는다

    EXPECT_EQ(ins->start_in_flight_.load(), 0);
}

TEST_F(StateManagerCtrlTest, OnStopAcceptsResponseWhileRequestInFlight) {
    ins->stop_in_flight_.store(1);

    lifecyclemgr::StopServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_STOP_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));

    ins->OnStop(&resp);

    EXPECT_EQ(ins->stop_in_flight_.load(), 0);
}

TEST_F(StateManagerCtrlTest, OnStopRelaysFailureVerdict) {
    ins->stop_in_flight_.store(1);

    lifecyclemgr::StopServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_STOP_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));

    ins->OnStop(&resp);

    EXPECT_EQ(ins->stop_in_flight_.load(), 0);
}

TEST_F(StateManagerCtrlTest, OnStopDropsPhantomAndLeavesCounterAlone) {
    ins->stop_in_flight_.store(0);

    lifecyclemgr::StopServiceResponse resp;
    resp.ret_code() = lifecyclemgr::ReturnCode::FAILED;
    EXPECT_CALL(*mock_transmitter, SendMessage(_, _)).Times(0);

    ins->OnStop(&resp);

    EXPECT_EQ(ins->stop_in_flight_.load(), 0);
}

TEST_F(StateManagerCtrlTest, StartAndStopCountersAreIndependent) {
    // stop 응답이 start 카운터를 갉아먹으면 뒤이은 진짜 start 응답이 유령으로
    // 버려진다. 두 카운터가 서로 독립인지 고정한다.
    ins->start_in_flight_.store(1);
    ins->stop_in_flight_.store(0);

    lifecyclemgr::StopServiceResponse stop_resp;
    stop_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    ins->OnStop(&stop_resp);          // 유령 — 버려진다

    EXPECT_EQ(ins->start_in_flight_.load(), 1) << "start 카운터는 그대로여야 한다";

    lifecyclemgr::StartServiceResponse start_resp;
    start_resp.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
    EXPECT_CALL(*mock_transmitter,
                SendMessage(MessageType::CTRL_START_SERVICE_RESPONSE, _))
        .WillOnce(Return(true));
    ins->OnStart(&start_resp);

    EXPECT_EQ(ins->start_in_flight_.load(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
