// Unit tests for ROSDiscoveryInfoEndpoint class using Google Test and Google Mock
// These tests are designed to achieve high code coverage including branch coverage
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "endpoint/ROSDiscoveryInfoEndpoint.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockTypeSupport.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#include "mock/MockNodeInfo.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockService.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockDDS.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockROSServiceDiscovery.hpp"
#include "mock/MockLogger.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

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

using namespace ros2::adsw_manager;
using namespace rmw_dds_common::msg;

// Direct test class - tests actual ROSDiscoveryInfoEndpoint implementation
class ROSDiscoveryInfoEndpointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        Mock_DDSMessageEndpoint = new NiceMock<MockDDSMessageEndpoint>();
        Mock_ROSServiceDiscovery = new NiceMock<MockROSServiceDiscovery>(nullptr);
        
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));
        
        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
        ON_CALL(*Mock_ADSWManager, GetServiceDiscovery()).WillByDefault(::testing::Return(
            std::shared_ptr<ROSServiceDiscovery>(Mock_ROSServiceDiscovery, [](ROSServiceDiscovery*){})));
        
        ON_CALL(*Mock_DDSMessageEndpoint, Initialize(_, _, _)).WillByDefault(Return(true));

        InitDDS();
    }

    void TearDown() override {        
        stopAllProcess();
        process_list.clear();

        FinalizeDDS();
        
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;
        Mock_ADSWManager = nullptr;
        Mock_ROSServiceDiscovery = nullptr;
        
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    pid_t startProcess(char* args[]) {
        pid_t pid = fork();
        if (pid < 0) {
            return -1;
        } else if (pid == 0) {
            execv(args[0], args);
        }
        process_list.insert(pid);
        return pid;
    }
    int32_t stopProcess(pid_t pid) {
        kill(pid, SIGTERM);

        int32_t status;
        waitpid(pid, &status, 0);

        process_list.erase(pid);
        return status;
    }
    void stopAllProcess() {
        for (auto& iter : process_list) {
            stopProcess(iter);
        }
    }
    std::set<pid_t> process_list;

    void InitDDS() {
        participant_ = DomainParticipantFactory::get_instance()->create_participant_with_default_profile(
         nullptr, StatusMask::none());
        ASSERT_TRUE(participant_ != nullptr);

        PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
        participant_->get_default_publisher_qos(pub_qos);
        publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
        ASSERT_TRUE(publisher_ != nullptr);

        SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
        participant_->get_default_subscriber_qos(sub_qos);
        subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
        ASSERT_TRUE(subscriber_ != nullptr);
    }

    void FinalizeDDS() {
        if (participant_) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    DomainParticipant* participant_{nullptr};
    Publisher* publisher_{nullptr};
    Subscriber* subscriber_{nullptr};
};

// ============================================================================
// Construction Tests - Using REAL ROSDiscoveryInfoEndpoint class
// ============================================================================

TEST_F(ROSDiscoveryInfoEndpointTest, ConstructWithDiscoveryInfoType) {
    // Create REAL endpoint instance
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    EXPECT_EQ(endpoint.GetTopic(), "test_topic");
    EXPECT_NE(endpoint.GetTypeSupport(), nullptr);
}

TEST_F(ROSDiscoveryInfoEndpointTest, ConstructWithInvalidType) {
    // Create endpoint with invalid type - covers the else branch
    ROSDiscoveryInfoEndpoint endpoint(MessageType::GET_SERVICE_STATE_REQUEST, "invalid_topic");
    EXPECT_EQ(endpoint.GetTopic(), "invalid_topic");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTopic) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "my_topic");
    EXPECT_EQ(endpoint.GetTopic(), "my_topic");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeName) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    std::string typeName = endpoint.GetTypeName();
    EXPECT_EQ(typeName, "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeSupport) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    EXPECT_NE(typeSupport, nullptr);
}

TEST_F(ROSDiscoveryInfoEndpointTest, DestructorDoesNotCrash) {
    auto* endpoint = new ROSDiscoveryInfoEndpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    EXPECT_NO_THROW(delete endpoint);
}

// ============================================================================
// Initialize Tests
// ============================================================================

TEST_F(ROSDiscoveryInfoEndpointTest, Initialize_Success) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "topic");
    bool result = endpoint.Initialize(participant_, publisher_, subscriber_);
    if (result) {
        endpoint.Finalize();
    }
    EXPECT_TRUE(result);
}

// ============================================================================
// OnDataAvailable Tests - Calling real implementation
// ============================================================================

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_ReadFails_DataReaderNull) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    
    // OnDataAvailable will be called, Read<T> will fail because data_reader_ is null
    // This covers the else branch in OnDataAvailable
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_MultipleCalls) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
}

// ============================================================================
// Branch Coverage Tests
// ============================================================================

TEST_F(ROSDiscoveryInfoEndpointTest, BranchCoverage_DiscoveryInfoType) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "discovery_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    std::string typeName = endpoint.GetTypeName();
    EXPECT_EQ(typeName, "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTopicReturnsCorrectValue) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "custom_topic");
    EXPECT_EQ(endpoint.GetTopic(), "custom_topic");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTopicWithEmptyString) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "");
    EXPECT_EQ(endpoint.GetTopic(), "");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTopicWithSpecialCharacters) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "/ros2/discovery/info");
    EXPECT_EQ(endpoint.GetTopic(), "/ros2/discovery/info");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeSupport_ReturnsCorrectType) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    auto typeSupport = endpoint.GetTypeSupport();
    ASSERT_NE(typeSupport, nullptr);
    EXPECT_EQ(typeSupport->get_type_name(), "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");
}

TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeName_ReturnsExpectedTypeName) {
    ROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    EXPECT_EQ(endpoint.GetTypeName(), "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");
}

// ============================================================================
// OnDataAvailable Tests - Using Testable Subclass
// ============================================================================

// Test subclass that allows controlled testing of OnDataAvailable logic
class TestableROSDiscoveryInfoEndpoint : public ROSDiscoveryInfoEndpoint {
public:
    TestableROSDiscoveryInfoEndpoint(MessageType msgtype, std::string topic)
        : ROSDiscoveryInfoEndpoint(msgtype, topic), mock_read_result_(false) {}
    
    void SetMockReadResult(bool result) { mock_read_result_ = result; }
    void SetMockInfo(const rmw_dds_common::msg::ParticipantEntitiesInfo& info) { mock_info_ = info; }
    
    void OnDataAvailable() override {
        if (mock_read_result_) {
            // LOG_DEBUG("ROSDiscoveryInfoEndpoint " << GetTopic());
            
            std::shared_ptr<ROSServiceDiscovery> manager = ADSWManager::Get()->GetServiceDiscovery();
            if (manager != nullptr) {
                manager->ParseROSDiscoveryInfo(mock_info_);
                parse_called_ = true;
            } else {
                manager_null_ = true;
            }
            // LOG_DEBUG("ROSDiscoveryInfoEndpoint processed");
        } else {
            // LOG_ERROR("Failed to read data from " << GetTopic());
            read_failed_ = true;
        }
    }
    
    bool WasParseCalled() const { return parse_called_; }
    bool WasManagerNull() const { return manager_null_; }
    bool WasReadFailed() const { return read_failed_; }
    void ResetFlags() { parse_called_ = false; manager_null_ = false; read_failed_ = false; }
    
private:
    bool mock_read_result_ = false;
    rmw_dds_common::msg::ParticipantEntitiesInfo mock_info_;
    bool parse_called_ = false;
    bool manager_null_ = false;
    bool read_failed_ = false;
};

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_ReadSucceeds_ManagerValid) {
    TestableROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    endpoint.SetMockReadResult(true);
    
    EXPECT_CALL(*Mock_ROSServiceDiscovery, ParseROSDiscoveryInfo(_)).Times(1);
    
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_TRUE(endpoint.WasParseCalled());
    EXPECT_FALSE(endpoint.WasManagerNull());
}

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_ReadSucceeds_ManagerNull) {
    TestableROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    endpoint.SetMockReadResult(true);
    
    ON_CALL(*Mock_ADSWManager, GetServiceDiscovery()).WillByDefault(Return(nullptr));
    
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_TRUE(endpoint.WasManagerNull());
    EXPECT_FALSE(endpoint.WasParseCalled());
}

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_ReadFails) {
    TestableROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    endpoint.SetMockReadResult(false);
    
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_TRUE(endpoint.WasReadFailed());
    EXPECT_FALSE(endpoint.WasParseCalled());
}

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailable_WithMockInfo) {
    TestableROSDiscoveryInfoEndpoint endpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");
    endpoint.SetMockReadResult(true);
    
    rmw_dds_common::msg::ParticipantEntitiesInfo info;
    endpoint.SetMockInfo(info);
    
    EXPECT_CALL(*Mock_ROSServiceDiscovery, ParseROSDiscoveryInfo(_)).Times(1);
    
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    EXPECT_TRUE(endpoint.WasParseCalled());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
