#include <gtest/gtest.h>
#include "endpoint/ROSDiscoveryInfoEndpoint.cpp"
#include "transmitter/DDSMessageEndpoint.cpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace ros2::adsw_manager;
using namespace rmw_dds_common::msg;



class ROSDiscoveryInfoEndpointTest : public ::testing::Test {
protected:
    void SetUp() override {
        ins = std::make_unique<ROSDiscoveryInfoEndpoint>(std::bind(&ROSDiscoveryInfoEndpointTest::MessageCallback, this, std::placeholders::_1));

        InitDDS();
    }

    void TearDown() override {        
        stopAllProcess();
        process_list.clear();

        ins.reset();

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
};

TEST_F(ROSDiscoveryInfoEndpointTest, Initialize_TC001) {
    EXPECT_TRUE(ins->Initialize(participant_, subscriber_, publisher_));
    ins->Finalize();
}

TEST_F(ROSDiscoveryInfoEndpointTest, ReceiveTest_TC001) {
    EXPECT_TRUE(ins->Initialize(participant_, subscriber_, publisher_));

    char* args[] = {"test/ROSDiscoveryInfoSender", 0};
    pid_t pid = startProcess(args);
    if (pid == -1) {
        std::cerr << "Failed to start test publisher" << std::endl;
        FAIL();
    }

    ParticipantEntitiesInfo info;
    EXPECT_TRUE(ReadMessage(2, info));


    stopProcess(pid);

    ins->Finalize();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}