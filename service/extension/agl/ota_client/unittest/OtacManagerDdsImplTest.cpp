#include <gtest/gtest.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "OtacManagerDdsImpl.hpp"
#include "ota_mgr/OtamClientServiceMsgPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;
using namespace std::chrono_literals;
using namespace ota_client::agl;

namespace {

class DdsEnv : public ::testing::Test {
protected:
    DomainParticipant* participant_{nullptr};
    Publisher*  publisher_{nullptr};
    Subscriber* subscriber_{nullptr};

    void SetUp() override {
        DomainParticipantQos pq = PARTICIPANT_QOS_DEFAULT;
        pq.name("otac_ut_participant");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pq);
        ASSERT_NE(participant_, nullptr);

        publisher_  = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        ASSERT_NE(publisher_,  nullptr);
        ASSERT_NE(subscriber_, nullptr);
    }

    void TearDown() override {
        // Writer/Reader/Topic은 각 테스트에서 OtacManagerDdsImpl::Close()로 먼저 해제
        if (participant_ && publisher_) {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }
        if (participant_ && subscriber_) {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }
        if (participant_) {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
};

} // anonymous

// 1) 한 인스턴스에서 writer+reader를 둘 다 만들고, Write → 콜백 수신
TEST_F(DdsEnv, LoopbackSingleInstanceInvokesCallback) {
    using namespace OtaManager;

    TypeSupport ts(new SetOtaClientRegistrationPubSubType());

    OtacManagerDdsImpl dds(participant_, publisher_, subscriber_, ts);
    ASSERT_TRUE(dds.Create("ut/otac/SetOtaClientRegistration", /*writer=*/true, /*reader=*/true));

    std::mutex m;
    std::condition_variable cv;
    bool got = false;
    SetOtaClientRegistration received;

    dds.SetOnMessage([&](std::shared_ptr<void> raw, const SampleInfo&) {
        auto* msg = static_cast<SetOtaClientRegistration*>(raw.get());
        {
            std::lock_guard<std::mutex> lk(m);
            received = *msg;
            got = true;
        }
        cv.notify_one();
    });

    auto to_send = std::make_shared<SetOtaClientRegistration>();
    to_send->client_name("tester");
    to_send->app_list().push_back("app1");
    to_send->app_list().push_back("app2");

    ASSERT_TRUE(dds.Write(to_send));

    std::unique_lock<std::mutex> lk(m);
    ASSERT_TRUE(cv.wait_for(lk, 2s, [&]{ return got; }));

    EXPECT_EQ(received.client_name(), "tester");
    ASSERT_EQ(received.app_list().size(), 2u);
    EXPECT_EQ(received.app_list()[0], "app1");
    EXPECT_EQ(received.app_list()[1], "app2");

    // ★ 리소스 정리
    dds.Close();
}

// 2) Writer-only ↔ Reader-only (같은 토픽) 사이 전달 확인
TEST_F(DdsEnv, WriterOnlyToReaderOnlyDeliversMessage) {
    using namespace OtaManager;

    const char* kTopic = "ut/otac/SetOtaClientRegistration";

    TypeSupport ts_w(new SetOtaClientRegistrationPubSubType());
    OtacManagerDdsImpl writer(participant_, publisher_, subscriber_, ts_w);
    ASSERT_TRUE(writer.Create(kTopic, /*writer=*/true, /*reader=*/false));

    TypeSupport ts_r(new SetOtaClientRegistrationPubSubType());
    OtacManagerDdsImpl reader(participant_, publisher_, subscriber_, ts_r);
    ASSERT_TRUE(reader.Create(kTopic, /*writer=*/false, /*reader=*/true));

    std::mutex m;
    std::condition_variable cv;
    bool got = false;
    SetOtaClientRegistration received;

    reader.SetOnMessage([&](std::shared_ptr<void> raw, const SampleInfo&) {
        auto* msg = static_cast<SetOtaClientRegistration*>(raw.get());
        {
            std::lock_guard<std::mutex> lk(m);
            received = *msg;
            got = true;
        }
        cv.notify_one();
    });

    auto to_send = std::make_shared<SetOtaClientRegistration>();
    to_send->client_name("writer_only");
    to_send->app_list().push_back("alpha");

    ASSERT_TRUE(writer.Write(to_send));

    std::unique_lock<std::mutex> lk(m);
    ASSERT_TRUE(cv.wait_for(lk, 2s, [&]{ return got; }));

    EXPECT_EQ(received.client_name(), "writer_only");
    ASSERT_EQ(received.app_list().size(), 1u);
    EXPECT_EQ(received.app_list()[0], "alpha");

    // ★ 리소스 정리(매칭 해제 대기 포함)
    reader.Close();
    writer.Close();
}
