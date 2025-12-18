/**
 * @file ConcreteEndpointIntegrationTest.cpp
 * @brief Integration tests for concrete endpoint classes in adsw_manager
 * 
 * This file contains integration tests that exercise the actual endpoint
 * implementations (StartServiceEndPoint, StopServiceEndPoint, etc.) to
 * improve coverage of the concrete endpoint OnDataAvailable() methods.
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
#include "endpoint/AdswServiceStatusEndPoint.hpp"
#include "endpoint/ROSDiscoveryInfoEndpoint.hpp"
#include "endpoint/MessageTypes.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "ros2/AdswServiceStatus.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"
#include "ros2/NodeEntitiesInfo.hpp"

#include "mock/MockLogger.hpp"
#include "mock/MockADSWManager.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockROSServiceController.hpp"

using namespace ros2::adsw_manager;
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

/**
 * @brief Base test fixture for concrete endpoint integration tests
 */
class ConcreteEndpointIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
                
        // Setup mock ADSWManager and dependencies
        mock_adsw_manager_ = std::make_shared<MockADSWManager>();
        mock_service_registry_ = std::make_shared<MockServiceRegistry>();
        mock_service_controller_ = std::make_shared<MockROSServiceController>();

        Mock_ADSWManager = mock_adsw_manager_.get();  // Set global mock pointer for stub methods
        ADSWManager::SetShared(mock_adsw_manager_);
             // Set global mock pointer for ADSWManager singleton
        Mock_ADSWManager = mock_adsw_manager_.get();
        Mock_ROSServiceController = mock_service_controller_.get();
        Mock_ServiceRegistry = mock_service_registry_.get();
        
        // Set up default mock behavior
        ON_CALL(*Mock_ADSWManager, GetServiceRegistry())
            .WillByDefault(Return(mock_service_registry_));
        ON_CALL(*Mock_ADSWManager, GetServiceController())
            .WillByDefault(Return(mock_service_controller_));

        ON_CALL(*Mock_ServiceRegistry, ProcessGetServiceStateRequest(_))
            .WillByDefault(Return(true));
        ON_CALL(*Mock_ROSServiceController, ProcessServiceStartRequest(_))
            .WillByDefault(Return(true));
        ON_CALL(*Mock_ROSServiceController, ProcessServiceStopRequest(_))
            .WillByDefault(Return(true));        

        // Create separate DDS participants for publisher and subscriber
        DomainParticipantQos pub_qos;
        pub_qos.name("ConcreteEndpointTest_Publisher");
        publisher_participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pub_qos);
        ASSERT_NE(publisher_participant_, nullptr);
        
        DomainParticipantQos sub_qos;
        sub_qos.name("ConcreteEndpointTest_Subscriber");
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
        // Reset the static shared_ptr and delete Mock_ADSWManager manually (we used empty deleter)
        ADSWManager::SetShared(nullptr);
        Mock_ADSWManager = nullptr;
        Mock_ROSServiceController = nullptr;
        Mock_ServiceRegistry = nullptr;
        
        mock_adsw_manager_.reset();
        mock_service_registry_.reset();
        mock_service_controller_.reset();
        
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
    
    std::shared_ptr<MockADSWManager> mock_adsw_manager_;
    std::shared_ptr<MockServiceRegistry> mock_service_registry_;
    std::shared_ptr<MockROSServiceController> mock_service_controller_;
};


/**
 * @brief Test StartServiceEndPoint OnDataAvailable() method
 * 
 * This test exercises the StartServiceEndPoint::OnDataAvailable() implementation
 * which calls Read<StartServiceRequest>() and processes the request.
 */
TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceRequest";
    
    // Create writer endpoint (simple endpoint for writing)
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create REAL reader endpoint (the one we want to test)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation for the service controller BEFORE sending data
    // The DDS listener will automatically trigger OnDataAvailable() when data arrives
    EXPECT_CALL(*Mock_ROSServiceController, ProcessServiceStartRequest(_))
        .WillOnce(Return(true));
    
    // Create and send a message
    lifecyclemgr::StartServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    
    // Wait for data to propagate and be processed by the DDS listener
    WaitForDiscovery(500);
    
    // Cleanup
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_TC02) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceRequest";
    
    // Create writer endpoint (simple endpoint for writing)
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create REAL reader endpoint (the one we want to test)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation for the service controller - return nullptr to test error path
    ON_CALL(*Mock_ADSWManager, GetServiceController())
        .WillByDefault(Return(nullptr));

    // Create and send a message
    lifecyclemgr::StartServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);

    // Trigger OnDataAvailable() - this exercises the REAL endpoint code!
    // StartServiceEndPoint::OnDataAvailable() will call Read<StartServiceRequest>()
    // and then call ProcessServiceStartRequest()
    reader_endpoint->OnDataAvailable();
    
    // Cleanup
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StartServiceEndPoint_OnDataAvailable_TC03) {
    const std::string topic = "mmf/lifecyclemgr/StartServiceRequest";
    
    // Create writer endpoint (simple endpoint for writing)
    auto writer_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create REAL reader endpoint (the one we want to test)
    auto reader_endpoint = std::make_unique<StartServiceEndPoint>(
        MessageType::START_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation BEFORE sending data - service controller will return false
    EXPECT_CALL(*Mock_ROSServiceController, ProcessServiceStartRequest(_))
        .WillOnce(Return(false));
    
    // Create and send a message
    lifecyclemgr::StartServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    
    // Wait for data to propagate and be processed
    WaitForDiscovery(500);
    
    // Cleanup
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test StopServiceEndPoint OnDataAvailable() method
 */
TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceRequest";
    
    auto writer_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
  
    // Setup expectation
    EXPECT_CALL(*Mock_ROSServiceController, ProcessServiceStopRequest(_))
        .WillOnce(Return(true));
        
    lifecyclemgr::StopServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}
TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable_TC02) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceRequest";
    
    auto writer_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation for the service controller - return nullptr to test error path
    ON_CALL(*Mock_ADSWManager, GetServiceController())
        .WillByDefault(Return(nullptr));
    
    lifecyclemgr::StopServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(ConcreteEndpointIntegrationTest, StopServiceEndPoint_OnDataAvailable_TC03) {
    const std::string topic = "mmf/lifecyclemgr/StopServiceRequest";
    
    auto writer_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceEndPoint>(
        MessageType::STOP_SERVICE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();

    // Setup expectation to return false
    EXPECT_CALL(*Mock_ROSServiceController, ProcessServiceStopRequest(_))
        .WillOnce(Return(false));

    lifecyclemgr::StopServiceRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);

    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test GetServiceStateEndPoint OnDataAvailable() method
 */
TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateRequest";
    
    auto writer_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation
    EXPECT_CALL(*Mock_ServiceRegistry, ProcessGetServiceStateRequest(_))
        .WillOnce(Return(true));
    
    lifecyclemgr::GetServiceStateRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_all_service(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}
TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable_TC02) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateRequest";
    
    auto writer_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    // Setup expectation - return nullptr to test error path
    ON_CALL(*Mock_ADSWManager, GetServiceController())
        .WillByDefault(Return(nullptr));

    lifecyclemgr::GetServiceStateRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_all_service(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}
TEST_F(ConcreteEndpointIntegrationTest, GetServiceStateEndPoint_OnDataAvailable_TC03) {
    const std::string topic = "mmf/lifecyclemgr/GetServiceStateRequest";
    
    auto writer_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateEndPoint>(
        MessageType::GET_SERVICE_STATE_REQUEST, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Setup expectation to return false
    EXPECT_CALL(*Mock_ServiceRegistry, ProcessGetServiceStateRequest(_))
        .WillOnce(Return(false));

    lifecyclemgr::GetServiceStateRequest message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.is_all_service(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test NotifyServiceStateEndPoint OnDataAvailable() method
 * Note: This endpoint only logs in OnDataAvailable(), doesn't process the message
 */
TEST_F(ConcreteEndpointIntegrationTest, NotifyServiceStateEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/lifecyclemgr/NotifyServiceState";
    
    auto writer_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateEndPoint>(
        MessageType::NOTIFY_SERVICE_STATE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    lifecyclemgr::NotifyServiceState message;
    message.os_type(lifecyclemgr::OsType::ROS2);
    message.service_list().push_back("test_service");
    message.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    // Trigger OnDataAvailable() - just exercises the logging code
    reader_endpoint->OnDataAvailable();
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test UpdateServiceListEndPoint OnDataAvailable() method
 * Note: This endpoint reads UpdateServiceListResponse (not Request)
 */
TEST_F(ConcreteEndpointIntegrationTest, UpdateServiceListEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/lifecyclemgr/UpdateServiceListResponse";
    
    auto writer_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_RESPONSE, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<UpdateServiceListEndPoint>(
        MessageType::UPDATE_SERVICE_LIST_RESPONSE, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    lifecyclemgr::UpdateServiceListResponse message;
    message.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test AdswServiceStatusEndPoint OnDataAvailable() method
 * Note: This endpoint only logs in OnDataAvailable()
 */
TEST_F(ConcreteEndpointIntegrationTest, AdswServiceStatusEndPoint_OnDataAvailable) {
    const std::string topic = "mmf/adswmgr/AdswServiceStatus";
    
    auto writer_endpoint = std::make_unique<ADSWServiceStatusEndPoint>(
        MessageType::ADSW_SERVICE_STATUS, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<ADSWServiceStatusEndPoint>(
        MessageType::ADSW_SERVICE_STATUS, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    adswmgr::AdswServiceStatus message;
    message.status_code(42);
    message.status_msg("Test status");
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

/**
 * @brief Test ROSDiscoveryInfoEndpoint OnDataAvailable() method
 * This endpoint processes ParticipantEntitiesInfo messages from ROS2 discovery
 */
TEST_F(ConcreteEndpointIntegrationTest, ROSDiscoveryInfoEndpoint_OnDataAvailable) {
    const std::string topic = "ros_discovery_info";
    
    auto writer_endpoint = std::make_unique<ROSDiscoveryInfoEndpoint>(
        MessageType::ROS2_DISCOVREY_INFO, topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<ROSDiscoveryInfoEndpoint>(
        MessageType::ROS2_DISCOVREY_INFO, topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Create a ParticipantEntitiesInfo message
    rmw_dds_common::msg::ParticipantEntitiesInfo message;
    
    // Set up a node entity
    rmw_dds_common::msg::NodeEntitiesInfo node_info;
    node_info.node_namespace("/test_namespace");
    node_info.node_name("test_node");
    
    // Add to participant entities
    message.node_entities_info_seq().push_back(node_info);
    
    ASSERT_TRUE(writer_endpoint->Write(&message));
    WaitForDiscovery(500);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
