/**
 * @file ConcreteEndpointIntegrationTest.cpp
 * @brief Integration tests for concrete endpoint classes in lifecycle_mgr
 * 
 * This file contains integration tests that exercise the actual endpoint
 * implementations (StartServiceEndPoint, StopServiceEndPoint, etc.) to
 * improve coverage of the concrete endpoint OnDataAvailable() methods.
 * 
 * These tests use real DDS communication to trigger OnDataAvailable() callbacks
 * and verify that the endpoints properly read data and delegate to the appropriate
 * manager components.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>
#include <memory>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "endpoint/StartServiceEndPoint.hpp"
#include "endpoint/StopServiceEndPoint.hpp"
#include "endpoint/GetServiceStateEndPoint.hpp"
#include "endpoint/NotifyServiceStateEndPoint.hpp"
#include "endpoint/UpdateServiceListEndPoint.hpp"
#include "endpoint/SetServiceModeEndPoint.hpp"
#include "endpoint/LifecycleDiagReportEndPoint.hpp"
// F013/F014 로 나중에 추가된 엔드포인트들. 이 파일이 처음 쓰일 당시에는 없었고
// 그 뒤로 아무도 추가하지 않아서, 열 개 전부 콜백 디스패치가 실행 0회였다.
#include "endpoint/CtrlStartServiceEndPoint.hpp"
#include "endpoint/CtrlStopServiceEndPoint.hpp"
#include "endpoint/CtrlSetServiceModeEndPoint.hpp"
#include "endpoint/CtrlStartContainerEndPoint.hpp"
#include "endpoint/CtrlStopContainerEndPoint.hpp"
#include "endpoint/CtrlSetRecoveryPolicyEndPoint.hpp"
#include "endpoint/SetRecoveryPolicyEndPoint.hpp"
#include "endpoint/StartContainerEndPoint.hpp"
#include "endpoint/StopContainerEndPoint.hpp"
#include "endpoint/NotifyContainerStateEndPoint.hpp"
#include "include/MessageType.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/LifecycleDiagReport.hpp"

#include "mock/mock_logger.hpp"
#include "mock/mock_manager_base.hpp"
#include "mock/mock_lifecycle_factory.hpp"
#include "mock/mock_state_manager.hpp"
#include "mock/mock_state_monitor.hpp"
#include "mock/mock_service_registry.hpp"
#include "mock/mock_lifecycle_manager.hpp"

using namespace lifecycle_manager;
using namespace lifecyclemgr;
using namespace eprosima::fastdds::dds;

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
using ::testing::NiceMock;

// Stub for LifecycleFactory::GetInstance() and getShared()
namespace lifecycle_manager {
    LifecycleFactory* LifecycleFactory::GetInstance() { 
        return Mock_LifecycleFactory; 
    }
    std::shared_ptr<LifecycleFactory> LifecycleFactory::getShared() { 
        return std::shared_ptr<LifecycleFactory>(Mock_LifecycleFactory, [](LifecycleFactory*){}); 
    }
}

/**
 * @brief Base test fixture for concrete endpoint integration tests
 */
class ConcreteEndpointIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock logger
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
        
        // Create mock lifecycle manager for StateManager/StateMonitor construction
        mock_lifecycle_manager_ = std::make_shared<NiceMock<MockLifecycleManager>>();
        
        // Create mock manager base
        mock_manager_base_ = new NiceMock<MockManagerBase>(*mock_lifecycle_manager_);
        Mock_ManagerBase = mock_manager_base_;
        
        // Create mock components
        mock_lifecycle_factory_ = new NiceMock<MockLifecycleFactory>();
        mock_state_manager_ = std::make_shared<NiceMock<MockStateManager>>(*mock_lifecycle_manager_);
        mock_state_monitor_ = std::make_shared<NiceMock<MockStateMonitor>>(*mock_lifecycle_manager_);
        mock_service_registry_ = std::make_shared<NiceMock<MockServiceRegistry>>();
        
        // Set global mock pointers
        Mock_LifecycleFactory = mock_lifecycle_factory_;
        Mock_StateManager = mock_state_manager_.get();
        Mock_StateMonitor = mock_state_monitor_.get();
        Mock_ServiceRegistry = mock_service_registry_.get();
        
        // Set up default mock behavior for LifecycleFactory
        ON_CALL(*Mock_LifecycleFactory, GetStateMgr())
            .WillByDefault(Return(mock_state_manager_));
        ON_CALL(*Mock_LifecycleFactory, GetMonitor())
            .WillByDefault(Return(mock_state_monitor_));
        ON_CALL(*Mock_LifecycleFactory, GetRegistry())
            .WillByDefault(Return(mock_service_registry_));
        
        // Create separate DDS participants for publisher and subscriber
        DomainParticipantQos pub_qos;
        pub_qos.name("LifecycleEndpointTest_Publisher");
        publisher_participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pub_qos);
        ASSERT_NE(publisher_participant_, nullptr);
        
        DomainParticipantQos sub_qos;
        sub_qos.name("LifecycleEndpointTest_Subscriber");
        subscriber_participant_ = DomainParticipantFactory::get_instance()->create_participant(0, sub_qos);
        ASSERT_NE(subscriber_participant_, nullptr);
        
        // Create publisher and subscriber
        publisher_ = publisher_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        ASSERT_NE(publisher_, nullptr);
        
        subscriber_ = subscriber_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        ASSERT_NE(subscriber_, nullptr);
    }

    void TearDown() override {
        if (publisher_participant_) {
            publisher_participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(publisher_participant_);
            publisher_participant_ = nullptr;
        }
        
        if (subscriber_participant_) {
            subscriber_participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(subscriber_participant_);
            subscriber_participant_ = nullptr;
        }
        
        // Clear mock pointers
        Mock_LifecycleFactory = nullptr;
        Mock_StateManager = nullptr;
        Mock_StateMonitor = nullptr;
        Mock_ServiceRegistry = nullptr;
        Mock_ManagerBase = nullptr;
        
        mock_state_manager_.reset();
        mock_state_monitor_.reset();
        mock_service_registry_.reset();
        mock_lifecycle_manager_.reset();
        
        delete mock_lifecycle_factory_;
        mock_lifecycle_factory_ = nullptr;
        
        delete mock_manager_base_;
        mock_manager_base_ = nullptr;
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    void WaitForDiscovery(int milliseconds = 500) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    DomainParticipant* publisher_participant_ = nullptr;
    DomainParticipant* subscriber_participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Subscriber* subscriber_ = nullptr;
    
    std::shared_ptr<NiceMock<MockLifecycleManager>> mock_lifecycle_manager_;
    NiceMock<MockLifecycleFactory>* mock_lifecycle_factory_ = nullptr;
    NiceMock<MockManagerBase>* mock_manager_base_ = nullptr;
    std::shared_ptr<NiceMock<MockStateManager>> mock_state_manager_;
    std::shared_ptr<NiceMock<MockStateMonitor>> mock_state_monitor_;
    std::shared_ptr<NiceMock<MockServiceRegistry>> mock_service_registry_;
};


// ============================================================================
// StartServiceEndPoint Tests
// ============================================================================

/**
 * @brief Test StartServiceEndPoint OnDataAvailable() - successful path
 * 
 * This test sends a StartServiceResponse message and verifies that
 * OnDataAvailable() reads the data and calls StateManager::OnStart()
 */
// ── *_TC001 계열이 콜백을 넘겨 리더를 만드는 이유 ───────────────────────────
//
// 각 EndPoint::OnDataAvailable() 은 수신한 메시지를 **생성자로 받은 콜백**에
// 넘긴다 (F013 리팩터링). 협력 객체를 직접 부르지 않는다. 프로덕션에서 그
// 배선은 LifecycleManager::Initialize() 의 handlers.on_* 람다가 한다.
//
// 원래 이 테스트들은 콜백 없이 리더를 만들어 놓고
// EXPECT_CALL(*Mock_StateManager, OnStart(_)) 를 걸어 두었다. 콜백이 없으니
// OnDataAvailable 은 아무것도 부르지 않아 기대가 절대 만족되지 않았다.
// 유닛 레인(integration-tests.regex 로 제외)과 통합 레인(runnable 미포함)
// 양쪽에서 빠져 있어 드러나지 않았을 뿐이다.
//
// 여기서 검증하는 것은 "쓴 샘플이 DDS 를 거쳐 리더에 도달하고, 디코딩된
// 메시지로 on_data 콜백이 불린다" 이다. 어느 콜백을 어디에 배선하는지는
// LifecycleManager 의 몫이라 이 파일의 관심사가 아니다.

TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceResponse";
    
    // Create writer endpoint for sending response
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create reader endpoint (the one we want to test)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic,
        [](lifecyclemgr::StartServiceResponse* msg) { LifecycleFactory::GetInstance()->GetStateMgr()->OnStart(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - OnStart should be called when data is received
    EXPECT_CALL(*Mock_StateManager, OnStart(_))
        .Times(AtLeast(1));
    
    // Create and send a StartServiceResponse message
    lifecyclemgr::StartServiceResponse message;
    message.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    
    // Wait for data to propagate and be processed by the DDS listener
    WaitForDiscovery(500);
    
    // Cleanup
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test StartServiceEndPoint OnDataAvailable() - null StateManager path
 */
TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceResponse";
    
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for StateManager to test error path
    ON_CALL(*Mock_LifecycleFactory, GetStateMgr())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::StartServiceResponse message;
    message.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    // Manually trigger OnDataAvailable to ensure code path is executed
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// StopServiceEndPoint Tests
// ============================================================================

/**
 * @brief Test StopServiceEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceResponse";
    
    auto writer_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_RESPONSE, topic,
        [](lifecyclemgr::StopServiceResponse* msg) { LifecycleFactory::GetInstance()->GetStateMgr()->OnStop(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - OnStop should be called
    EXPECT_CALL(*Mock_StateManager, OnStop(_))
        .Times(AtLeast(1));
    
    lifecyclemgr::StopServiceResponse message;
    message.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test StopServiceEndPoint OnDataAvailable() - null StateManager path
 */
TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceResponse";
    
    auto writer_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for StateManager
    ON_CALL(*Mock_LifecycleFactory, GetStateMgr())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::StopServiceResponse message;
    message.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// GetServiceStateEndPoint Tests
// ============================================================================

/**
 * @brief Test GetServiceStateEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateResponse";
    
    auto writer_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_RESPONSE, topic,
        [](lifecyclemgr::GetServiceStateResponse* msg) { Mock_StateMonitor->UpdateServicesState(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - UpdateServicesState should be called
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::GetServiceStateResponse*>()))
        .Times(AtLeast(1));
    
    lifecyclemgr::GetServiceStateResponse message;

    // Add a service status
    message.service_list().push_back("service1");
    message.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test GetServiceStateEndPoint OnDataAvailable() - null StateMonitor path
 */
TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateResponse";
    
    auto writer_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for StateMonitor
    ON_CALL(*Mock_LifecycleFactory, GetMonitor())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::GetServiceStateResponse message;
    message.service_list().push_back("service1");
    message.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// NotifyServiceStateEndPoint Tests
// ============================================================================

/**
 * @brief Test NotifyServiceStateEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, NotifyServiceStateEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/NotifyServiceState";
    
    auto writer_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic,
        [](lifecyclemgr::NotifyServiceState* msg) { Mock_StateMonitor->UpdateServicesState(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - UpdateServicesState should be called
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::NotifyServiceState*>()))
        .Times(AtLeast(1));
    
    lifecyclemgr::NotifyServiceState message;
    message.mw_type(lifecyclemgr::MwType::ROS2);
    
    // Add a service state
    message.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    message.service_list().push_back("test_service");
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test NotifyServiceStateEndPoint OnDataAvailable() - null StateMonitor path
 */
TEST_F(ConcreteEndpointIntegrationTest, NotifyServiceStateEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/NotifyServiceState";
    
    auto writer_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for StateMonitor
    ON_CALL(*Mock_LifecycleFactory, GetMonitor())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::NotifyServiceState message;
    message.mw_type(lifecyclemgr::MwType::ROS2);
    message.service_list().push_back("test_service");
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// UpdateServiceListEndPoint Tests
// ============================================================================

/**
 * @brief Test UpdateServiceListEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, UpdateServiceListEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/UpdateServiceListRequest";
    
    auto writer_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic,
        [](lifecyclemgr::UpdateServiceListRequest* msg) { Mock_ServiceRegistry->UpdateServiceList(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - UpdateServiceList should be called
    EXPECT_CALL(*Mock_ServiceRegistry, UpdateServiceList(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    
    lifecyclemgr::UpdateServiceListRequest message;

    lifecyclemgr::Service service;
    service.name("new_service");
    service.service_type(lifecyclemgr::ServiceType::NORMAL);
    service.recovery_policy(lifecyclemgr::RecoveryPolicy::ALWAYS);
    service.is_enabled(true);
    service.mw_type(lifecyclemgr::MwType::ROS2);

    message.service_list().push_back(service);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test UpdateServiceListEndPoint OnDataAvailable() - null ServiceRegistry path
 */
TEST_F(ConcreteEndpointIntegrationTest, UpdateServiceListEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/UpdateServiceListRequest";
    
    auto writer_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for ServiceRegistry
    ON_CALL(*Mock_LifecycleFactory, GetRegistry())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::UpdateServiceListRequest message;
    lifecyclemgr::Service service;
    service.name("new_service");
    service.service_type(lifecyclemgr::ServiceType::NORMAL);
    service.recovery_policy(lifecyclemgr::RecoveryPolicy::ALWAYS);
    service.is_enabled(true);
    service.mw_type(lifecyclemgr::MwType::ROS2);

    message.service_list().push_back(service);
        
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// SetServiceModeEndPoint Tests
// ============================================================================

/**
 * @brief Test SetServiceModeEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, SetServiceModeEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/SetServiceModeRequest";
    
    auto writer_endpoint = std::make_unique<SetServiceModeEndPoint>(
        MessageType::SET_SERVICE_MODE_REQUST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<SetServiceModeEndPoint>(
        MessageType::SET_SERVICE_MODE_REQUST, topic,
        [](lifecyclemgr::SetServiceModeRequest* msg) { LifecycleFactory::GetInstance()->GetStateMgr()->SetServiceMode(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - SetServiceMode should be called
    EXPECT_CALL(*Mock_StateManager, SetServiceMode(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    
    lifecyclemgr::SetServiceModeRequest message;
    message.mw_type(lifecyclemgr::MwType::ROS2);
    message.is_group(true);
    message.service_list().push_back("test_service");
    message.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test SetServiceModeEndPoint OnDataAvailable() - null StateManager path
 */
TEST_F(ConcreteEndpointIntegrationTest, SetServiceModeEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/SetServiceModeRequest";
    
    auto writer_endpoint = std::make_unique<SetServiceModeEndPoint>(
        MessageType::SET_SERVICE_MODE_REQUST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<SetServiceModeEndPoint>(
        MessageType::SET_SERVICE_MODE_REQUST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Return nullptr for StateManager
    ON_CALL(*Mock_LifecycleFactory, GetStateMgr())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::SetServiceModeRequest message;
    message.mw_type(lifecyclemgr::MwType::ROS2);
    message.is_group(true);
    message.service_list().push_back("test_service");
    message.service_mode() = lifecyclemgr::ServiceMode::NORMAL;

    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// LifecycleDiagReportEndPoint Tests
// ============================================================================

/**
 * @brief Test LifecycleDiagReportEndPoint OnDataAvailable() - successful path
 */
TEST_F(ConcreteEndpointIntegrationTest, LifecycleDiagReportEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/LifecycleDiagReport";
    
    auto writer_endpoint = std::make_unique<LifecycleDiagReportEndPoint>(
        MessageType::LIFECYCLE_DIAG_REPORT, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<LifecycleDiagReportEndPoint>(
        MessageType::LIFECYCLE_DIAG_REPORT, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Create a LifecycleDiagReport message with service list
    lifecyclemgr::LifecycleDiagReport message;
    
    lifecyclemgr::ServiceStatus status1;
    status1.name("service1");
    status1.service_type(lifecyclemgr::ServiceType::NORMAL);
    status1.service_state(lifecyclemgr::ServiceState::RUNNING);
    status1.is_enabled(true);
    message.service_list().push_back(status1);
    
    lifecyclemgr::ServiceStatus status2;
    status2.name("service2");
    status2.service_type(lifecyclemgr::ServiceType::NORMAL);
    status2.service_state(lifecyclemgr::ServiceState::RUNNING);
    status2.is_enabled(false);
    message.service_list().push_back(status2);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    // Manually trigger to ensure code path is executed
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test LifecycleDiagReportEndPoint OnDataAvailable() - empty service list
 */
TEST_F(ConcreteEndpointIntegrationTest, LifecycleDiagReportEndPoint_OnDataAvailable_TC002) {
    const std::string topic = "mmf/lifecyclemgr/LifecycleDiagReport";
    
    auto writer_endpoint = std::make_unique<LifecycleDiagReportEndPoint>(
        MessageType::LIFECYCLE_DIAG_REPORT, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<LifecycleDiagReportEndPoint>(
        MessageType::LIFECYCLE_DIAG_REPORT, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Create an empty LifecycleDiagReport message
    lifecyclemgr::LifecycleDiagReport message;
    // service_list is empty
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// Multiple Messages Tests
// ============================================================================

/**
 * @brief Test sending multiple messages to verify continuous data availability
 */
TEST_F(ConcreteEndpointIntegrationTest, NotifyServiceState_MultipleMessages) {
    const std::string topic = "mmf/lifecyclemgr/NotifyServiceStateMulti";
    
    auto writer_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic,
        [](lifecyclemgr::NotifyServiceState* msg) { Mock_StateMonitor->UpdateServicesState(msg); });
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Expect UpdateServicesState to be called at least 3 times
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::NotifyServiceState*>()))
        .Times(AtLeast(3));
    
    // Send multiple messages
    for (int i = 0; i < 3; ++i) {
        lifecyclemgr::NotifyServiceState message;
        message.mw_type(lifecyclemgr::MwType::ROS2);
        message.service_list().push_back("service_" + std::to_string(i));
        message.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
        
        ASSERT_TRUE(writer_endpoint->Write(&message));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    WaitForDiscovery(500);  
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}


// ============================================================================
// Read Failure Tests (no data available)
// ============================================================================

/**
 * @brief Test OnDataAvailable when Read fails (no data available)
 */
TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceResponseEmpty";
    
    // Create only reader endpoint (no writer, so no data)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    // OnStart should NOT be called since Read will fail (no data)
    EXPECT_CALL(*Mock_StateManager, OnStart(_))
        .Times(0);
    
    // Call OnDataAvailable when there's no data - should handle gracefully
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceResponseEmpty";
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    EXPECT_CALL(*Mock_StateManager, OnStop(_))
        .Times(0);
    
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateResponseEmpty";
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::GetServiceStateResponse*>()))
        .Times(0);
    
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, NotifyServiceStateEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/NotifyServiceStateEmpty";
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::NotifyServiceState*>()))
        .Times(0);
    
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, UpdateServiceListEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/UpdateServiceListRequestEmpty";
    
    auto reader_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    EXPECT_CALL(*Mock_ServiceRegistry, UpdateServiceList(_))
        .Times(0);
    
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, SetServiceModeEndPoint_OnDataAvailable_ReadFail) {
    const std::string topic = "mmf/lifecyclemgr/SetServiceModeRequestEmpty";
    
    auto reader_endpoint = std::make_unique<SetServiceModeEndPoint>(
        MessageType::SET_SERVICE_MODE_REQUST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery(100);
    
    EXPECT_CALL(*Mock_StateManager, SetServiceMode(_))
        .Times(0);
    
    EXPECT_NO_THROW(reader_endpoint->OnDataAvailable());
    
    reader_endpoint->Finalize();
}


// ═══════════════════════════════════════════════════════════════════════════
// F013/F014 엔드포인트 열 개의 콜백 디스패치
//
// 왜 미커버였나: OnDataAvailable() 의 `if (on_data_cb_) { on_data_cb_(msg); }`
// 는 그 앞의 Read<T>() 가 성공해야 도달한다. Read 는 reader_ 가 nullptr 이면
// 즉시 false 라, 실제 DataReader 가 붙지 않은 유닛 테스트에서는 절대 못 간다
// (CtrlEndPointTest 가 그 사실을 문서화해 두었다). 위쪽 여섯 종은 이 파일에서
// 실 DDS 왕복으로 덮여 있는데, 뒤에 추가된 이 열 개는 아무도 추가하지 않았다.
//
// 이 경로가 끊기면 monitor 가 보낸 ctrl/* 요청과 액추에이터가 보낸 컨테이너
// 응답이 조용히 버려진다 — 토픽은 맞고 수신도 되는데 핸들러만 안 불리는,
// 로그로는 잘 안 보이는 종류의 고장이다.
//
// 목으로 라우팅하지 않고 람다로 직접 받는다. 콜백이 불렸는지뿐 아니라
// 페이로드가 실제로 왕복했는지까지 봐야 "수신은 됐지만 내용이 깨졌다"를
// 구분할 수 있다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// 실 DDS 왕복은 타이밍이 있다. WaitForDiscovery(500) 한 번으로 끝내지 않고
// 조건이 설 때까지 짧게 여러 번 본다 — 느린 CI 에서 간헐 실패하지 않게.
template <typename Pred>
bool WaitUntil(Pred pred, int total_ms = 3000, int step_ms = 50) {
    for (int waited = 0; waited < total_ms; waited += step_ms) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
    }
    return pred();
}

} // namespace

TEST_F(ConcreteEndpointIntegrationTest, CtrlStartServiceEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/StartService";
    auto writer = std::make_unique<CtrlStartServiceEndPoint>(
        MessageType::CTRL_START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    auto reader = std::make_unique<CtrlStartServiceEndPoint>(
        MessageType::CTRL_START_SERVICE_REQUEST, topic,
        [&](lifecyclemgr::CtrlStartServiceRequest* m) { got = m->service_list(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlStartServiceRequest msg;
    msg.service_list().push_back("autoware_perception");
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"autoware_perception"}));

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, CtrlStopServiceEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/StopService";
    auto writer = std::make_unique<CtrlStopServiceEndPoint>(
        MessageType::CTRL_STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    auto reader = std::make_unique<CtrlStopServiceEndPoint>(
        MessageType::CTRL_STOP_SERVICE_REQUEST, topic,
        [&](lifecyclemgr::CtrlStopServiceRequest* m) { got = m->service_list(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlStopServiceRequest msg;
    msg.service_list().push_back("autoware_planning");
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"autoware_planning"}));

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, CtrlSetServiceModeEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/SetServiceMode";
    auto writer = std::make_unique<CtrlSetServiceModeEndPoint>(
        MessageType::CTRL_SET_SERVICE_MODE_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    lifecyclemgr::ServiceMode got_mode = lifecyclemgr::ServiceMode::NORMAL;
    auto reader = std::make_unique<CtrlSetServiceModeEndPoint>(
        MessageType::CTRL_SET_SERVICE_MODE_REQUEST, topic,
        [&](lifecyclemgr::CtrlSetServiceModeRequest* m) {
            got = m->service_list(); got_mode = m->service_mode(); ++hits;
        });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlSetServiceModeRequest msg;
    msg.service_list().push_back("ota_target");
    msg.service_mode(lifecyclemgr::ServiceMode::OTA_MODE);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"ota_target"}));
    // 모드가 깨져서 오면 OTA 중에 서비스가 평소처럼 재시작된다.
    EXPECT_EQ(got_mode, lifecyclemgr::ServiceMode::OTA_MODE);

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, CtrlSetRecoveryPolicyEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/SetRecoveryPolicy";
    auto writer = std::make_unique<CtrlSetRecoveryPolicyEndPoint>(
        MessageType::CTRL_SET_RECOVERY_POLICY_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    lifecyclemgr::RecoveryPolicy got_policy = lifecyclemgr::RecoveryPolicy::R_DEFAULT;
    auto reader = std::make_unique<CtrlSetRecoveryPolicyEndPoint>(
        MessageType::CTRL_SET_RECOVERY_POLICY_REQUEST, topic,
        [&](lifecyclemgr::CtrlSetRecoveryPolicyRequest* m) {
            got = m->service_list(); got_policy = m->recovery_policy(); ++hits;
        });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlSetRecoveryPolicyRequest msg;
    msg.service_list().push_back("worker");
    msg.recovery_policy(lifecyclemgr::RecoveryPolicy::ALWAYS);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"worker"}));
    // 정책이 깨져 오면 재시작 루프가 잘못 돈다 — F014 사고의 원인이 이 축이었다.
    EXPECT_EQ(got_policy, lifecyclemgr::RecoveryPolicy::ALWAYS);

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, CtrlStartContainerEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/StartContainer";
    auto writer = std::make_unique<CtrlStartContainerEndPoint>(
        MessageType::CTRL_START_CONTAINER_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    auto reader = std::make_unique<CtrlStartContainerEndPoint>(
        MessageType::CTRL_START_CONTAINER_REQUEST, topic,
        [&](lifecyclemgr::CtrlStartContainerRequest* m) { got = m->container_list(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlStartContainerRequest msg;
    msg.container_list().push_back("autoware-perception");
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"autoware-perception"}));

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, CtrlStopContainerEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/ctrl/StopContainer";
    auto writer = std::make_unique<CtrlStopContainerEndPoint>(
        MessageType::CTRL_STOP_CONTAINER_REQUEST, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> got;
    auto reader = std::make_unique<CtrlStopContainerEndPoint>(
        MessageType::CTRL_STOP_CONTAINER_REQUEST, topic,
        [&](lifecyclemgr::CtrlStopContainerRequest* m) { got = m->container_list(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::CtrlStopContainerRequest msg;
    msg.container_list().push_back("autoware-planning");
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, (std::vector<std::string>{"autoware-planning"}));

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StartContainerEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/StartContainerResponse";
    auto writer = std::make_unique<StartContainerEndPoint>(
        MessageType::START_CONTAINER_RESPONSE, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    lifecyclemgr::ReturnCode got = lifecyclemgr::ReturnCode::SUCCESS;
    auto reader = std::make_unique<StartContainerEndPoint>(
        MessageType::START_CONTAINER_RESPONSE, topic,
        [&](lifecyclemgr::StartContainerResponse* m) { got = m->ret_code(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::StartContainerResponse msg;
    msg.ret_code(lifecyclemgr::ReturnCode::FAILED);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    // 실패가 성공으로 뒤바뀌면 모니터가 안 뜬 컨테이너를 떴다고 표시한다.
    EXPECT_EQ(got, lifecyclemgr::ReturnCode::FAILED);

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StopContainerEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/StopContainerResponse";
    auto writer = std::make_unique<StopContainerEndPoint>(
        MessageType::STOP_CONTAINER_RESPONSE, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    lifecyclemgr::ReturnCode got = lifecyclemgr::ReturnCode::FAILED;
    auto reader = std::make_unique<StopContainerEndPoint>(
        MessageType::STOP_CONTAINER_RESPONSE, topic,
        [&](lifecyclemgr::StopContainerResponse* m) { got = m->ret_code(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::StopContainerResponse msg;
    msg.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, lifecyclemgr::ReturnCode::SUCCESS);

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, SetRecoveryPolicyEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/SetRecoveryPolicyResponse";
    auto writer = std::make_unique<SetRecoveryPolicyEndPoint>(
        MessageType::SET_RECOVERY_POLICY_RESPONSE, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    lifecyclemgr::ReturnCode got = lifecyclemgr::ReturnCode::FAILED;
    auto reader = std::make_unique<SetRecoveryPolicyEndPoint>(
        MessageType::SET_RECOVERY_POLICY_RESPONSE, topic,
        [&](lifecyclemgr::SetRecoveryPolicyResponse* m) { got = m->ret_code(); ++hits; });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::SetRecoveryPolicyResponse msg;
    msg.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    EXPECT_EQ(got, lifecyclemgr::ReturnCode::SUCCESS);

    reader->Finalize();
    writer->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, NotifyContainerStateEndPoint_DispatchesCallback) {
    const std::string topic = "mmf/lifecyclemgr/NotifyContainerState";
    auto writer = std::make_unique<NotifyContainerStateEndPoint>(
        MessageType::NOTIFY_CONTAINER_STATE, topic);
    ASSERT_TRUE(writer->Initialize(publisher_participant_, publisher_, nullptr));

    std::atomic<int> hits{0};
    std::vector<std::string> names;
    std::vector<lifecyclemgr::ContainerState> states;
    std::vector<int32_t> codes;
    auto reader = std::make_unique<NotifyContainerStateEndPoint>(
        MessageType::NOTIFY_CONTAINER_STATE, topic,
        [&](lifecyclemgr::NotifyContainerState* m) {
            names = m->container_list();
            states = m->container_state_list();
            codes = m->exit_code_list();
            ++hits;
        });
    ASSERT_TRUE(reader->Initialize(subscriber_participant_, nullptr, subscriber_));
    WaitForDiscovery();

    lifecyclemgr::NotifyContainerState msg;
    msg.container_list().push_back("web");
    msg.container_list().push_back("db");
    msg.container_state_list().push_back(lifecyclemgr::ContainerState::CONTAINER_RUNNING);
    msg.container_state_list().push_back(lifecyclemgr::ContainerState::CONTAINER_STOPPED);
    msg.exit_code_list().push_back(0);
    msg.exit_code_list().push_back(137);
    ASSERT_TRUE(writer->Write(&msg));

    EXPECT_TRUE(WaitUntil([&] { return hits.load() > 0; }));
    // 세 리스트는 인덱스로 짝지어진 병렬 배열이다. 전선을 건너면서 순서가
    // 어긋나면 모니터가 A 컨테이너 상태로 B 를 표시한다.
    ASSERT_EQ(names.size(), 2u);
    ASSERT_EQ(states.size(), 2u);
    ASSERT_EQ(codes.size(), 2u);
    EXPECT_EQ(names[0], "web");
    EXPECT_EQ(states[0], lifecyclemgr::ContainerState::CONTAINER_RUNNING);
    EXPECT_EQ(codes[0], 0);
    EXPECT_EQ(names[1], "db");
    EXPECT_EQ(states[1], lifecyclemgr::ContainerState::CONTAINER_STOPPED);
    EXPECT_EQ(codes[1], 137);

    reader->Finalize();
    writer->Finalize();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
