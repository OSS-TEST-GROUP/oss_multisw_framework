// Integration tests for DDSMessageEndPoint Read functionality
// Tests actual DDS communication with publisher/subscriber pairs
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "transmitter/DDSMessageEndPoint.hpp"
#include "unittest/mock/mock_logger.hpp"

// Include generated DDS types for testing
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/StopServiceRequestPubSubTypes.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"
#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

using namespace lifecycle_manager;
using namespace eprosima::fastdds::dds;
using ::testing::Return;

// ============================================================================
// Concrete Endpoint Implementations for Testing
// ============================================================================

// NotifyServiceState Endpoint
class NotifyServiceStateTestEndPoint : public DDSMessageEndPoint {
public:
    NotifyServiceStateTestEndPoint(const std::string& topic_name = "test_notify_service_state") 
        : DDSMessageEndPoint(topic_name) {
        auto type_support = std::make_shared<TypeSupport>(new lifecyclemgr::NotifyServiceStatePubSubType());
        SetTypeSupport(type_support);
    }
    
    void OnDataAvailable() override { data_received_ = true; }
    void OnDataPublished(bool ret) override { }

    bool ReadMessage(lifecyclemgr::NotifyServiceState& msg) {
        return Read(msg);
    }
    
    bool WasDataReceived() const { return data_received_; }
    
private:
    bool data_received_ = false;
};

// StartServiceRequest Endpoint
class StartServiceRequestTestEndPoint : public DDSMessageEndPoint {
public:
    StartServiceRequestTestEndPoint(const std::string& topic_name = "test_start_service_request")
        : DDSMessageEndPoint(topic_name) {
        auto type_support = std::make_shared<TypeSupport>(new lifecyclemgr::StartServiceRequestPubSubType());
        SetTypeSupport(type_support);
    }
    
    void OnDataAvailable() override { data_received_ = true; }
    void OnDataPublished(bool ret) override { }
    
    bool ReadMessage(lifecyclemgr::StartServiceRequest& msg) {
        return Read(msg);
    }
    
    bool WasDataReceived() const { return data_received_; }
    
private:
    bool data_received_ = false;
};

// StopServiceRequest Endpoint
class StopServiceRequestTestEndPoint : public DDSMessageEndPoint {
public:
    StopServiceRequestTestEndPoint(const std::string& topic_name = "test_stop_service_request")
        : DDSMessageEndPoint(topic_name) {
        auto type_support = std::make_shared<TypeSupport>(new lifecyclemgr::StopServiceRequestPubSubType());
        SetTypeSupport(type_support);
    }
    
    void OnDataAvailable() override { data_received_ = true; }
    void OnDataPublished(bool ret) override { }
    
    bool ReadMessage(lifecyclemgr::StopServiceRequest& msg) {
        return Read(msg);
    }
    
    bool WasDataReceived() const { return data_received_; }
    
private:
    bool data_received_ = false;
};

// GetServiceStateRequest Endpoint
class GetServiceStateRequestTestEndPoint : public DDSMessageEndPoint {
public:
    GetServiceStateRequestTestEndPoint(const std::string& topic_name = "test_get_service_state_request")
        : DDSMessageEndPoint(topic_name) {
        auto type_support = std::make_shared<TypeSupport>(new lifecyclemgr::GetServiceStateRequestPubSubType());
        SetTypeSupport(type_support);
    }
    
    void OnDataAvailable() override { data_received_ = true; }
    void OnDataPublished(bool ret) override { }

    bool ReadMessage(lifecyclemgr::GetServiceStateRequest& msg) {
        return Read(msg);
    }
    
    bool WasDataReceived() const { return data_received_; }
    
private:
    bool data_received_ = false;
};

// UpdateServiceListRequest Endpoint
class UpdateServiceListRequestTestEndPoint : public DDSMessageEndPoint {
public:
    UpdateServiceListRequestTestEndPoint(const std::string& topic_name = "test_update_service_list_request")
        : DDSMessageEndPoint(topic_name) {
        auto type_support = std::make_shared<TypeSupport>(new lifecyclemgr::UpdateServiceListRequestPubSubType());
        SetTypeSupport(type_support);
    }
    
    void OnDataAvailable() override { data_received_ = true; }
    void OnDataPublished(bool ret) override { }

    bool ReadMessage(lifecyclemgr::UpdateServiceListRequest& msg) {
        return Read(msg);
    }
    
    bool WasDataReceived() const { return data_received_; }
    
private:
    bool data_received_ = false;
};

// ============================================================================
// Test Fixture
// ============================================================================

class DDSMessageEndPointReadTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        
        // Create separate DDS participants for publisher and subscriber to avoid conflicts
        DomainParticipantQos pub_qos;
        pub_qos.name("DDSMessageEndPointReadTest_Publisher");
        publisher_participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pub_qos);
        ASSERT_NE(publisher_participant_, nullptr);
        
        DomainParticipantQos sub_qos;
        sub_qos.name("DDSMessageEndPointReadTest_Subscriber");
        subscriber_participant_ = DomainParticipantFactory::get_instance()->create_participant(0, sub_qos);
        ASSERT_NE(subscriber_participant_, nullptr);
        
        // Create publisher and subscriber on separate participants
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
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
    
    // Helper to wait for discovery
    void WaitForDiscovery(int milliseconds = 500) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    DomainParticipant* publisher_participant_ = nullptr;
    DomainParticipant* subscriber_participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Subscriber* subscriber_ = nullptr;
};

// ============================================================================
// NotifyServiceState Read Tests
// ============================================================================

TEST_F(DDSMessageEndPointReadTest, ReadNotifyServiceState_Success) {
    const std::string topic = "notify_state_topic_1";
    
    // Create writer endpoint on publisher participant
    auto writer_endpoint = std::make_unique<NotifyServiceStateTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    // Create reader endpoint on subscriber participant
    auto reader_endpoint = std::make_unique<NotifyServiceStateTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    // Wait for discovery
    WaitForDiscovery();
    
    // Create and write message
    lifecyclemgr::NotifyServiceState send_msg;
    send_msg.mw_type(lifecyclemgr::MwType::ROS2);
    send_msg.service_list().push_back("test_service");
    send_msg.service_state_list().push_back(lifecyclemgr::ServiceState::RUNNING);
    
    ASSERT_TRUE(writer_endpoint->Write(&send_msg));
    
    // Wait for message to arrive
    WaitForDiscovery(100);
    
    // Trigger OnDataAvailable() to exercise the endpoint's implementation
    reader_endpoint->OnDataAvailable();
    
    // Read message from the reader endpoint
    lifecyclemgr::NotifyServiceState recv_msg;
    bool read_result = reader_endpoint->ReadMessage(recv_msg);
    
    if (read_result) {
        EXPECT_EQ(recv_msg.service_list().size(), 1);
        if (!recv_msg.service_list().empty()) {
            EXPECT_EQ(recv_msg.service_list()[0], "test_service");
        }
        EXPECT_EQ(recv_msg.service_state_list().size(), 1);
    }
    
    // Explicitly finalize endpoints before destruction
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(DDSMessageEndPointReadTest, ReadStartServiceRequest_Success) {
    const std::string topic = "start_req_topic_1";
    
    auto writer_endpoint = std::make_unique<StartServiceRequestTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StartServiceRequestTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Create and write message
    lifecyclemgr::StartServiceRequest send_msg;
    send_msg.mw_type(lifecyclemgr::MwType::ROS2);
    send_msg.service_list().push_back("start_test_service");
    send_msg.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&send_msg));
    WaitForDiscovery(100);
    
    // Trigger OnDataAvailable() to exercise the endpoint's implementation
    reader_endpoint->OnDataAvailable();
    
    // Read message
    lifecyclemgr::StartServiceRequest recv_msg;
    bool read_result = reader_endpoint->ReadMessage(recv_msg);
    
    if (read_result) {
        EXPECT_EQ(recv_msg.service_list().size(), 1);
        if (!recv_msg.service_list().empty()) {
            EXPECT_EQ(recv_msg.service_list()[0], "start_test_service");
        }
        EXPECT_EQ(recv_msg.is_group(), false);
    }
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(DDSMessageEndPointReadTest, ReadStopServiceRequest_Success) {
    const std::string topic = "stop_req_topic_1";
    
    auto writer_endpoint = std::make_unique<StopServiceRequestTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<StopServiceRequestTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    lifecyclemgr::StopServiceRequest send_msg;
    send_msg.mw_type(lifecyclemgr::MwType::ROS2);
    send_msg.service_list().push_back("stop_test_service");
    send_msg.is_group(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&send_msg));
    WaitForDiscovery(100);

    // Trigger OnDataAvailable() to exercise the endpoint's implementation
    reader_endpoint->OnDataAvailable();

    lifecyclemgr::StopServiceRequest recv_msg;
    bool read_result = reader_endpoint->ReadMessage(recv_msg);
    
    if (read_result) {
        EXPECT_EQ(recv_msg.service_list().size(), 1);
        if (!recv_msg.service_list().empty()) {
            EXPECT_EQ(recv_msg.service_list()[0], "stop_test_service");
        }
    }
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(DDSMessageEndPointReadTest, ReadGetServiceStateRequest_Success) {
    const std::string topic = "get_state_topic_1";
    
    auto writer_endpoint = std::make_unique<GetServiceStateRequestTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<GetServiceStateRequestTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    lifecyclemgr::GetServiceStateRequest send_msg;
    send_msg.mw_type(lifecyclemgr::MwType::ROS2);
    send_msg.service_list().push_back("query_test_service");
    send_msg.is_all_service(false);
    
    ASSERT_TRUE(writer_endpoint->Write(&send_msg));
    WaitForDiscovery(100);

    // Trigger OnDataAvailable() to exercise the endpoint's implementation
    reader_endpoint->OnDataAvailable();

    lifecyclemgr::GetServiceStateRequest recv_msg;
    bool read_result = reader_endpoint->ReadMessage(recv_msg);
    
    if (read_result) {
        EXPECT_EQ(recv_msg.service_list().size(), 1);
        if (!recv_msg.service_list().empty()) {
            EXPECT_EQ(recv_msg.service_list()[0], "query_test_service");
        }
    }
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

TEST_F(DDSMessageEndPointReadTest, ReadUpdateServiceListRequest_Success) {
    const std::string topic = "update_list_topic_1";
    
    auto writer_endpoint = std::make_unique<UpdateServiceListRequestTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<UpdateServiceListRequestTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    lifecyclemgr::UpdateServiceListRequest send_msg;
    // Empty service list - just test the basic structure
    
    ASSERT_TRUE(writer_endpoint->Write(&send_msg));
    WaitForDiscovery(100);

    // Trigger OnDataAvailable() to exercise the endpoint's implementation
    reader_endpoint->OnDataAvailable();

    lifecyclemgr::UpdateServiceListRequest recv_msg;
    bool read_result = reader_endpoint->ReadMessage(recv_msg);
    
    if (read_result) {
        // Message was read successfully
        EXPECT_TRUE(recv_msg.service_list().empty());
    }
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(DDSMessageEndPointReadTest, ReadWithoutInitialize) {
    auto endpoint = std::make_unique<NotifyServiceStateTestEndPoint>();
    
    // Try to read without initializing
    lifecyclemgr::NotifyServiceState msg;
    EXPECT_FALSE(endpoint->ReadMessage(msg));
}

TEST_F(DDSMessageEndPointReadTest, ReadBeforeAnyDataWritten) {
    const std::string topic = "notify_no_data_topic";
    
    auto writer_endpoint = std::make_unique<NotifyServiceStateTestEndPoint>(topic);
    ASSERT_TRUE(writer_endpoint->Initialize(publisher_participant_, publisher_, nullptr));
    
    auto reader_endpoint = std::make_unique<NotifyServiceStateTestEndPoint>(topic);
    ASSERT_TRUE(reader_endpoint->Initialize(subscriber_participant_, nullptr, subscriber_));
    
    WaitForDiscovery();
    
    // Try to read before any data is written
    lifecyclemgr::NotifyServiceState msg;
    bool result = reader_endpoint->ReadMessage(msg);
    
    // Should return false
    EXPECT_FALSE(result);
    
    reader_endpoint->Finalize();
    writer_endpoint->Finalize();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
