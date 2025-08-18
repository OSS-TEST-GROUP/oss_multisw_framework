#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "NotifyAppStateWriter.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "agl/NotifyAppStatePubSubTypes.hpp"

#include "../common/AppUtils.hpp"

using namespace adsw_manager::agl;
using namespace eprosima::fastdds::dds;

class NotifyAppStateWriterTest : public ::testing::Test {
protected:
    NotifyAppStateWriter test_writer;
    static DomainParticipant* participant_;
    static Publisher* publisher_;

    static void SetUpTestSuite() {
        DomainParticipantQos qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(nullptr, participant_);

        publisher_   = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(nullptr, publisher_);
    }

    static void TearDownTestSuite() {
        if (participant_) {
            if (publisher_) { participant_->delete_publisher(publisher_); publisher_ = nullptr; }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
};

DomainParticipant* NotifyAppStateWriterTest::participant_ = nullptr;
Publisher* NotifyAppStateWriterTest::publisher_ = nullptr;

// ----------------------------- Initialize -----------------------------

TEST_F(NotifyAppStateWriterTest, NotifyAppStateWriter_Initialize) {

    EXPECT_TRUE(test_writer.Initialize(publisher_, participant_));

}

// ----------------------------- Publish -----------------------------

TEST_F(NotifyAppStateWriterTest, NotifyAppStateWriter_Publish) {

    ASSERT_TRUE(test_writer.Initialize(publisher_, participant_))
        << "Initialize failed -> writer_가 nullptr일 가능성 큼";

    adsw_manager_agl::NotifyAppState msg;
    msg.app_name() = "hello";
    msg.app_state() = adsw_manager_agl::AppState::DETACHED;

    EXPECT_TRUE(test_writer.Publish(msg));

}