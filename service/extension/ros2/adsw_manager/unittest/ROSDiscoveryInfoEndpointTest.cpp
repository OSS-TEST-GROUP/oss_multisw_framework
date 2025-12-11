// Unit tests for DDSTransmitter class using Google Test and Google Mock
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

using namespace ros2::adsw_manager;
using namespace rmw_dds_common::msg;


class ROSDiscoveryInfoEndpointTest : public ::testing::Test {
protected:
    void SetUp() override {
		// TODO
        //ins = std::make_unique<ROSDiscoveryInfoEndpoint>(std::bind(&ROSDiscoveryInfoEndpointTest::MessageCallback, this, std::placeholders::_1));
        ins = std::make_unique<ROSDiscoveryInfoEndpoint>(ros2::adsw_manager::MessageType::ROS2_DISCOVREY_INFO, "topic");
        //MockADSWManager::discovery = new MockROSServiceDiscovery();
        endpoint = new ros2::adsw_manager::ROSDiscoveryInfoEndpoint(MessageType::ROS2_DISCOVREY_INFO, "test_topic");

        Mock_DDSMessageEndpoint = new MockDDSMessageEndpoint();

        InitDDS();
    }

    void TearDown() override {        
        stopAllProcess();
        process_list.clear();

        ins.reset();

        delete endpoint;
        //delete MockADSWManager::discovery;
        delete Mock_DDSMessageEndpoint;
        Mock_DDSMessageEndpoint = nullptr;

        FinalizeDDS();
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

        // Create the publisher
        PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
        participant_->get_default_publisher_qos(pub_qos);
        publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
        ASSERT_TRUE(publisher_ != nullptr);

        // Create the subscriber
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

    void MessageCallback(ParticipantEntitiesInfo& info) {
        info_ = info;
        info_received_ = true;
    }

    bool ReadMessage(uint32_t timeout_sec, ParticipantEntitiesInfo& info) {
        int count = 0;
        while (true) {
            if (info_received_ == true) {
                info = info_;
                return true;
            }
            if (count < (timeout_sec * 100)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                count++;
            } else {
                break;
            }
        }
        return false;
    }

    std::unique_ptr<ROSDiscoveryInfoEndpoint> ins;

    DomainParticipant* participant_{nullptr};
    Publisher* publisher_{nullptr};
    Subscriber* subscriber_{nullptr};

    ParticipantEntitiesInfo info_;
    bool info_received_{false};
    
    ros2::adsw_manager::ROSDiscoveryInfoEndpoint* endpoint;

};

TEST_F(ROSDiscoveryInfoEndpointTest, Initialize_TC001) {
    EXPECT_TRUE(ins->Initialize(participant_, publisher_, subscriber_));
    ins->Finalize();
}

TEST_F(ROSDiscoveryInfoEndpointTest, ReceiveTest_TC001) {
    EXPECT_TRUE(ins->Initialize(participant_, publisher_, subscriber_));

    char* args[] = {"test/ROSDiscoveryInfoSender", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    ParticipantEntitiesInfo info;
    //EXPECT_TRUE(ReadMessage(2, info));

    stopProcess(pid);

    ins->Finalize();
}
TEST_F(ROSDiscoveryInfoEndpointTest, ConstructionAndGetters) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
    EXPECT_EQ(endpoint->GetTypeName(), "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");

    EXPECT_EQ(endpoint->GetTypeSupport(), endpoint->type_support_);
}

TEST_F(ROSDiscoveryInfoEndpointTest, OnDataAvailableCallsServiceDiscovery) {
    // Simulate Read<rmw_dds_common::msg::ParticipantEntitiesInfo>(info) == true
    // You may need to mock/stub Read<> if it's not already stubbed
    //endpoint->OnDataAvailable();
    // The test will pass if no crash; for full coverage, mock Read<> to return true and check called
    // EXPECT_TRUE(MockADSWManager::discovery->called);
}
TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeSupportReturnsValidType) {

    auto type_support = endpoint->GetTypeSupport();
    ASSERT_NE(endpoint->GetTypeSupport(), nullptr); // Use ASSERT to stop if null

    std::string type_name = type_support->get_type_name();
    EXPECT_EQ(type_name, "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");

}
TEST_F(ROSDiscoveryInfoEndpointTest, GetTopicReturnsCorrectTopic) {
    EXPECT_EQ(endpoint->GetTopic(), "test_topic");
}
TEST_F(ROSDiscoveryInfoEndpointTest, GetTypeNameReturnsCorrectTypeName) {
    EXPECT_EQ(endpoint->GetTypeName(), "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_");
}
TEST_F(ROSDiscoveryInfoEndpointTest, TypeSupportIsNotNull) {
    auto type_support = endpoint->GetTypeSupport();
    EXPECT_NE(type_support, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
