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
TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_TC001) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceResponse";
    
    // Create writer endpoint for sending response
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create reader endpoint (the one we want to test)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_RESPONSE, topic);
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
        MessageType::STOP_SERVICE_RESPONSE, topic);
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
        MessageType::GET_SERVICE_STATE_RESPONSE, topic);
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
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - UpdateServicesState should be called
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::NotifyServiceState*>()))
        .Times(AtLeast(1));
    
    lifecyclemgr::NotifyServiceState message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    
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
    message.os_type(lifecyclemgr::OsType::ROS2);
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
        MessageType::UPDATE_SERVICE_LIST_REQUEST, topic);
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
    service.os_type(lifecyclemgr::OsType::ROS2);

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
    service.os_type(lifecyclemgr::OsType::ROS2);

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
        MessageType::SET_SERVICE_MODE_REQUST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation - SetServiceMode should be called
    EXPECT_CALL(*Mock_StateManager, SetServiceMode(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    
    lifecyclemgr::SetServiceModeRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
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
    message.os_type(lifecyclemgr::OsType::ROS2);
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
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Expect UpdateServicesState to be called at least 3 times
    EXPECT_CALL(*Mock_StateMonitor, UpdateServicesState(A<lifecyclemgr::NotifyServiceState*>()))
        .Times(AtLeast(3));
    
    // Send multiple messages
    for (int i = 0; i < 3; ++i) {
        lifecyclemgr::NotifyServiceState message;
        message.os_type(lifecyclemgr::OsType::ROS2);
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


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
