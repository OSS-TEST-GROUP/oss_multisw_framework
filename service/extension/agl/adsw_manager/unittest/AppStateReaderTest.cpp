#include <gtest/gtest.h>
#include "service_monitor/AppStateReader.hpp"
#include "agl/String_PubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <thread>

#include "TestPrinter.hpp"

using namespace adsw_manager::agl;
using namespace eprosima::fastdds::dds;
using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::seconds;

class AppStateReaderTest : public ::testing::Test {
protected:
    DomainParticipant* participant = nullptr;

    void SetUp() override {
        DomainParticipantQos qos;
        qos.name("reader_participant");
        participant = DomainParticipantFactory::get_instance()->create_participant(0, qos);
        ASSERT_NE(participant, nullptr);
    }

    void TearDown() override {
        if (participant) {
            DomainParticipantFactory::get_instance()->delete_participant(participant);
        }
    }
};

// ----------------------------- GetLastReceivedTime -----------------------------

TEST_F(AppStateReaderTest, InitializesWithoutReceivedSample) {
    AppStateReader reader(participant, "test_app");
    auto last = reader.GetLastReceivedTime();
    EXPECT_EQ(last, steady_clock::time_point::min());
}

// ----------------------------- on_data_available -----------------------------

TEST_F(AppStateReaderTest, OnDataAvailable_UpdatesLastReceivedTime_WhenValidSampleArrives) {
    using namespace eprosima::fastdds::dds;
    AppStateReader reader(participant, "test_app");
    auto before = reader.GetLastReceivedTime();

    DomainParticipantQos pub_qos;
    pub_qos.name("publisher_participant");
    DomainParticipant* pub_part =
        DomainParticipantFactory::get_instance()->create_participant(0, pub_qos);
    ASSERT_NE(pub_part, nullptr);

    TypeSupport type(new std_msgs::msg::dds_::String_PubSubType());
    type.register_type(pub_part);

    Topic* topic = pub_part->create_topic(
        "rt/app_status/test_app", type.get_type_name(), TOPIC_QOS_DEFAULT);
    ASSERT_NE(topic, nullptr);

    Publisher* publisher = pub_part->create_publisher(PUBLISHER_QOS_DEFAULT);
    ASSERT_NE(publisher, nullptr);

    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    dw_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    dw_qos.history().depth = 5;

    DataWriter* writer = publisher->create_datawriter(topic, dw_qos);
    ASSERT_NE(writer, nullptr);

    // Writer–Reader 매칭될 때까지 대기
    PublicationMatchedStatus pub_status{};
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        writer->get_publication_matched_status(pub_status);
        if (pub_status.current_count > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_GT(pub_status.current_count, 0) << "Writer-Reader 매칭 실패";

    // 샘플 전송
    std_msgs::msg::dds_::String_ msg;
    msg.data("ping");
    ASSERT_EQ(writer->write(&msg), RETCODE_OK);

    // 갱신 확인(최대 2초 폴링)
    bool updated = false;
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < end) {
        if (reader.GetLastReceivedTime() > before) { updated = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    EXPECT_TRUE(updated) << "last_received_time_ not updated after publish";

    // 정리
    publisher->delete_datawriter(writer);
    pub_part->delete_publisher(publisher);
    pub_part->delete_topic(topic);
    DomainParticipantFactory::get_instance()->delete_participant(pub_part);
}
