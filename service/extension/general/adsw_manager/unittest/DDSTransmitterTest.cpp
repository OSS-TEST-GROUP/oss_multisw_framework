// general adsw_manager — DDSMessageEndpoint / DDSTransmitter 유닛테스트
//
// 두 파일 모두 커버리지 0% 였다. 실제 DDS participant 없이는 못 돈다고 보기
// 쉽지만, 상당 부분은 participant 이전/이후의 방어 로직이다.
//   - 아직 Initialize 하지 않은 상태의 Write / Read / Finalize
//   - null participant / publisher / subscriber 로 들어온 Initialize
//   - 엔드포인트 등록·조회(토픽 → 엔드포인트 맵)
//   - 리스너 콜백(on_publication_matched / on_subscription_matched)
//
// 이 경계들이야말로 크래시가 나는 곳이고, DDS 없이 검증할 수 있다.
// 실제 pub→sub 왕복은 통합 레인(ConcreteEndpointIntegrationTest) 소관이다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/MessageQueue.hpp"
#include "endpoint/MessageTypes.hpp"

#include "mock/MockLogger.hpp"
// MessageQueue 가 ADSWManager 싱글턴을 참조한다 (전송 루프에서 토픽/트랜스미터
// 조회). 링크 심으로 끊는다 — 실물을 링크하면 DDS participant 가 뜬다.
#include "mock/MockADSWManager.hpp"

using general::adsw_manager::DDSMessageEndpoint;
using general::adsw_manager::DDSTransmitter;
using general::adsw_manager::MessageQueue;
using general::adsw_manager::MessageType;
using general::adsw_manager::MsgType;
using general::adsw_manager::ScopedMockADSWManager;
using general::adsw_manager::ScopedMockADSWManager;

namespace {

// DDSMessageEndpoint 는 추상 클래스다. 순수 가상 4개만 채우면 DDS 없이
// 나머지 구현부를 그대로 돌릴 수 있다.
class FakeEndpoint : public DDSMessageEndpoint {
public:
    explicit FakeEndpoint(std::string topic = "test/topic",
                          std::string type  = "TestType")
        : topic_(std::move(topic)), type_(std::move(type)) {}

    std::string GetTopic() override { return topic_; }
    std::string GetTypeName() override { return type_; }
    std::shared_ptr<TypeSupport> GetTypeSupport() override { return type_support_; }
    void OnDataAvailable() override { ++on_data_calls_; }

    int on_data_calls_ = 0;

private:
    std::string topic_;
    std::string type_;
    std::shared_ptr<TypeSupport> type_support_;
};

class DDSEndpointTest : public ::testing::Test {
protected:
    void SetUp() override { logger_ = std::make_unique<ScopedMockLogger>(); }
    void TearDown() override { logger_.reset(); }
    std::unique_ptr<ScopedMockLogger> logger_;
};

} // namespace

// ── Initialize 이전 상태 ────────────────────────────────────────────────────

TEST_F(DDSEndpointTest, ConstructedEndpointExposesTopicAndType) {
    FakeEndpoint ep("mmf/general/Foo", "FooType");
    EXPECT_EQ(ep.GetTopic(), "mmf/general/Foo");
    EXPECT_EQ(ep.GetTypeName(), "FooType");
    EXPECT_EQ(ep.GetTypeSupport(), nullptr);
}

TEST_F(DDSEndpointTest, WriteBeforeInitializeFailsInsteadOfCrashing) {
    FakeEndpoint ep;
    int payload = 0;
    EXPECT_FALSE(ep.Write(&payload));
}

TEST_F(DDSEndpointTest, WriteNullPayloadFails) {
    FakeEndpoint ep;
    EXPECT_FALSE(ep.Write(nullptr));
}

TEST_F(DDSEndpointTest, ReadBeforeInitializeFails) {
    FakeEndpoint ep;
    int value = 0;
    EXPECT_FALSE(ep.Read(value));
}

TEST_F(DDSEndpointTest, FinalizeBeforeInitializeIsSafe) {
    FakeEndpoint ep;
    EXPECT_NO_THROW(ep.Finalize());
}

TEST_F(DDSEndpointTest, DoubleFinalizeIsSafe) {
    FakeEndpoint ep;
    ep.Finalize();
    EXPECT_NO_THROW(ep.Finalize());
}

TEST_F(DDSEndpointTest, InitializeWithNullParticipantFails) {
    FakeEndpoint ep;
    EXPECT_FALSE(ep.Initialize(nullptr, nullptr, nullptr));
}

TEST_F(DDSEndpointTest, DestructionAfterFailedInitializeIsSafe) {
    // 소멸자가 정리 경로를 타는데, 초기화된 적 없는 포인터를 만지면 안 된다.
    EXPECT_NO_THROW({
        FakeEndpoint ep;
        (void)ep.Initialize(nullptr, nullptr, nullptr);
    });
}

// ── 리스너 콜백 ─────────────────────────────────────────────────────────────

TEST_F(DDSEndpointTest, PublicationMatchedCallbackIsSafe) {
    FakeEndpoint ep;
    eprosima::fastdds::dds::PublicationMatchedStatus st;
    st.current_count = 1;
    st.current_count_change = 1;
    EXPECT_NO_THROW(ep.on_publication_matched(nullptr, st));

    st.current_count = 0;
    st.current_count_change = -1;
    EXPECT_NO_THROW(ep.on_publication_matched(nullptr, st));
}

TEST_F(DDSEndpointTest, SubscriptionMatchedCallbackIsSafe) {
    FakeEndpoint ep;
    eprosima::fastdds::dds::SubscriptionMatchedStatus st;
    st.current_count = 1;
    st.current_count_change = 1;
    EXPECT_NO_THROW(ep.on_subscription_matched(nullptr, st));

    st.current_count = 0;
    st.current_count_change = -1;
    EXPECT_NO_THROW(ep.on_subscription_matched(nullptr, st));
}

TEST_F(DDSEndpointTest, DataAvailableCallbackDelegatesToOnDataAvailable) {
    FakeEndpoint ep;
    EXPECT_NO_THROW(ep.on_data_available(nullptr));
    EXPECT_EQ(ep.on_data_calls_, 1);
}

// current_count_change 는 ±1 만 오는 게 정상이지만 코드에 else 분기가 있다.
// 실제로 밟히는지 아무도 확인한 적이 없어 0회였다. QoS 가 바뀌거나 리스너가
// 배치로 불릴 때 0 이 올 수 있으므로, 그 경로가 크래시하지 않는지 고정한다.
TEST_F(DDSEndpointTest, PublicationMatchedWithUnexpectedChangeIsIgnored) {
    FakeEndpoint ep;
    eprosima::fastdds::dds::PublicationMatchedStatus st;
    st.current_count = 3;
    st.current_count_change = 0;
    EXPECT_NO_THROW(ep.on_publication_matched(nullptr, st));

    st.current_count_change = 2;
    EXPECT_NO_THROW(ep.on_publication_matched(nullptr, st));
}

TEST_F(DDSEndpointTest, SubscriptionMatchedWithUnexpectedChangeIsIgnored) {
    FakeEndpoint ep;
    eprosima::fastdds::dds::SubscriptionMatchedStatus st;
    st.current_count = 3;
    st.current_count_change = 0;
    EXPECT_NO_THROW(ep.on_subscription_matched(nullptr, st));

    st.current_count_change = -2;
    EXPECT_NO_THROW(ep.on_subscription_matched(nullptr, st));
}

// reader/writer 가 없을 때의 대기량 조회. 두 함수 모두 실행 카운트가 0 이었다.
TEST_F(DDSEndpointTest, PendingCountsBeforeInitializeAreZero) {
    FakeEndpoint ep;
    EXPECT_EQ(ep.GetPendingReadData(), 0U);
    EXPECT_EQ(ep.GetPendingWriteData(), 0U);
}

// ═══════════════════════════════════════════════════════════════════════════
// DDSTransmitter — 엔드포인트 레지스트리
//
// Initialize() 는 실제 participant 를 만든다. 여기서는 그 이전/이후의
// 등록·조회·전송 게이트만 본다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {
class DDSTransmitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        tx_ = std::make_unique<DDSTransmitter>();
    }
    void TearDown() override {
        tx_.reset();
        logger_.reset();
    }
    std::unique_ptr<ScopedMockLogger> logger_;
    std::unique_ptr<DDSTransmitter>   tx_;
};
} // namespace

TEST_F(DDSTransmitterTest, FreshTransmitterHasNoDomainOrEndpoints) {
    EXPECT_EQ(tx_->GetDomain(), nullptr);
    EXPECT_EQ(tx_->GetPublisher(), nullptr);
    EXPECT_EQ(tx_->GetSubscriber(), nullptr);
    EXPECT_EQ(tx_->GetMessageEndpoint("anything"), nullptr);
}

TEST_F(DDSTransmitterTest, AddNullEndpointRejected) {
    EXPECT_FALSE(tx_->AddMessageEndpoint(nullptr));
}

TEST_F(DDSTransmitterTest, AddEndpointRegistersByTopicEvenBeforeInitialize) {
    // 등록은 participant 와 무관하다 — 토픽 → 엔드포인트 맵에 넣을 뿐이고,
    // 실제 writer/reader 연결은 나중에 Initialize 가 한다.
    auto ep = std::make_shared<FakeEndpoint>("mmf/general/A", "AType");
    EXPECT_TRUE(tx_->AddMessageEndpoint(ep));
    EXPECT_EQ(tx_->GetMessageEndpoint("mmf/general/A"), ep);
}

TEST_F(DDSTransmitterTest, AddEndpointWithSameTopicReplaces) {
    auto a = std::make_shared<FakeEndpoint>("mmf/general/A", "AType");
    auto b = std::make_shared<FakeEndpoint>("mmf/general/A", "BType");
    EXPECT_TRUE(tx_->AddMessageEndpoint(a));
    EXPECT_TRUE(tx_->AddMessageEndpoint(b));
    EXPECT_EQ(tx_->GetMessageEndpoint("mmf/general/A"), b);
}

TEST_F(DDSTransmitterTest, SendMessageToRegisteredButUninitializedEndpointFails) {
    // 엔드포인트는 찾지만 writer 가 없으므로 Write 가 실패해야 한다 (크래시 아님).
    auto ep = std::make_shared<FakeEndpoint>("mmf/general/A", "AType");
    tx_->AddMessageEndpoint(ep);
    int payload = 0;
    EXPECT_FALSE(tx_->SendMessage(MessageType::NOTIFY_SERVICE_STATE, &payload));
}

TEST_F(DDSTransmitterTest, GetUnknownTopicReturnsNull) {
    EXPECT_EQ(tx_->GetMessageEndpoint(""), nullptr);
    EXPECT_EQ(tx_->GetMessageEndpoint("no/such/topic"), nullptr);
}

TEST_F(DDSTransmitterTest, SendMessageWithoutEndpointFails) {
    int payload = 0;
    EXPECT_FALSE(tx_->SendMessage(MessageType::NOTIFY_SERVICE_STATE, &payload));
}

TEST_F(DDSTransmitterTest, SendMessageWithNullPayloadFails) {
    EXPECT_FALSE(tx_->SendMessage(MessageType::NOTIFY_SERVICE_STATE, nullptr));
}

TEST_F(DDSTransmitterTest, FinalizeBeforeInitializeIsSafe) {
    EXPECT_NO_THROW(tx_->Finalize());
}

TEST_F(DDSTransmitterTest, DoubleFinalizeIsSafe) {
    tx_->Finalize();
    EXPECT_NO_THROW(tx_->Finalize());
}

// 기존 Finalize 테스트는 엔드포인트가 하나도 없는 상태만 봤다. 그래서 등록된
// 엔드포인트를 순회하며 Finalize 를 전파하는 루프가 0회였다.
TEST_F(DDSTransmitterTest, FinalizePropagatesToRegisteredEndpoints) {
    auto a = std::make_shared<FakeEndpoint>("mmf/general/A", "AType");
    auto b = std::make_shared<FakeEndpoint>("mmf/general/B", "BType");
    tx_->AddMessageEndpoint(a);
    tx_->AddMessageEndpoint(b);

    EXPECT_NO_THROW(tx_->Finalize());

    // Finalize 는 등록을 해제하지 않는다 — DDS 엔티티만 정리한다.
    EXPECT_EQ(tx_->GetMessageEndpoint("mmf/general/A"), a);
    EXPECT_EQ(tx_->GetMessageEndpoint("mmf/general/B"), b);
}

// ═══════════════════════════════════════════════════════════════════════════
// MessageQueue — 발행 스레드
//
// 이 파일은 이미 이 바이너리에 링크돼 있었지만 테스트가 하나도 없었다.
// 커버리지상 Start/Stop/SendMessage 는 다른 테스트가 간접적으로 밟고 있었고
// PublishingLoop 본문은 통째로 0회였다. 원인은 경합이다: 스레드를 띄우자마자
// Stop() 이 이겨서 `while (!stop_publisher_)` 가 처음부터 거짓이 된다.
//
// 그래서 여기서는 **Start() 하기 전에 큐에 메시지를 넣는다.** 그러면 스레드가
// 깨어났을 때 cv 술어(`!queue.empty()`)가 이미 참이라 블로킹 없이 본문으로
// 들어간다. 소비 완료 신호는 루프가 부르는 ADSWManager::GetTopic() 을
// 가로채서 받으므로 sleep 으로 추측하지 않는다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {
class MessageQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        mgr_    = std::make_unique<ScopedMockADSWManager>();
        tx_     = std::make_shared<DDSTransmitter>();

        ON_CALL(**mgr_, GetTransmitter()).WillByDefault(::testing::Return(tx_));

        // 널 토픽을 돌려주면 LOG_DEBUG 의 `<< topic` 이 UB 다. 항상 실제
        // 문자열을 준다. 반환이 const char* 라 수명은 topic_ 이 유지한다.
        ON_CALL(**mgr_, GetTopic(::testing::_))
            .WillByDefault(::testing::Invoke([this](MessageType) -> const char* {
                {
                    std::lock_guard<std::mutex> lk(consumed_m_);
                    ++consumed_;
                }
                consumed_cv_.notify_all();
                return topic_.c_str();
            }));
    }

    void TearDown() override {
        // 스레드를 먼저 세운다. mock 이 살아 있는 동안 끝나야 한다.
        q_.Stop();
        tx_.reset();
        mgr_.reset();
        logger_.reset();
    }

    // 루프가 메시지를 n 건 집을 때까지 기다린다. 시간이 아니라 사건을 기다린다.
    bool WaitConsumed(int n) {
        std::unique_lock<std::mutex> lk(consumed_m_);
        return consumed_cv_.wait_for(lk, std::chrono::seconds(5),
                                     [this, n] { return consumed_ >= n; });
    }

    void Enqueue(int count) {
        for (int i = 0; i < count; ++i) {
            MsgType msg{MessageType::NOTIFY_SERVICE_STATE, &payload_};
            q_.SendMessage(msg);
        }
    }

    std::unique_ptr<ScopedMockLogger>      logger_;
    std::unique_ptr<ScopedMockADSWManager> mgr_;
    std::shared_ptr<DDSTransmitter>        tx_;
    MessageQueue                           q_;
    std::string                            topic_ = "mmf/general/QueueTopic";
    int                                    payload_ = 0;

    std::mutex              consumed_m_;
    std::condition_variable consumed_cv_;
    int                     consumed_ = 0;
};
} // namespace

TEST_F(MessageQueueTest, StartTwiceIsRejected) {
    EXPECT_TRUE(q_.Start());
    EXPECT_FALSE(q_.Start()) << "이미 도는 스레드가 있는데 또 시작됐다";
}

TEST_F(MessageQueueTest, RestartAfterStopIsAllowed) {
    ASSERT_TRUE(q_.Start());
    q_.Stop();
    // join() 뒤에는 joinable() 이 거짓이므로 다시 띄울 수 있어야 한다.
    EXPECT_TRUE(q_.Start());
}

TEST_F(MessageQueueTest, StopWithoutStartIsSafe) {
    EXPECT_NO_THROW(q_.Stop());
    EXPECT_NO_THROW(q_.Stop());
}

TEST_F(MessageQueueTest, PublishingLoopDrainsMessageWhenTopicHasNoEndpoint) {
    ASSERT_EQ(tx_->GetMessageEndpoint(topic_), nullptr);

    Enqueue(1);
    ASSERT_TRUE(q_.Start());

    ASSERT_TRUE(WaitConsumed(1)) << "PublishingLoop 이 큐를 비우지 않았다";
}

TEST_F(MessageQueueTest, PublishingLoopSurvivesFailedWrite) {
    // 등록은 됐지만 Initialize 를 안 한 엔드포인트 — Write 가 false 를 낸다.
    auto ep = std::make_shared<FakeEndpoint>(topic_, "QueueType");
    ASSERT_TRUE(tx_->AddMessageEndpoint(ep));

    Enqueue(1);
    ASSERT_TRUE(q_.Start());

    ASSERT_TRUE(WaitConsumed(1));
    // 실패해도 루프는 죽지 않고 다음 메시지를 계속 받아야 한다.
    Enqueue(1);
    EXPECT_TRUE(WaitConsumed(2)) << "Write 실패 후 루프가 멈췄다";
}

TEST_F(MessageQueueTest, PublishingLoopDrainsAllQueuedMessagesInOneWake) {
    Enqueue(3);
    ASSERT_TRUE(q_.Start());
    EXPECT_TRUE(WaitConsumed(3));
}

TEST_F(MessageQueueTest, StopWakesIdleLoop) {
    Enqueue(1);
    ASSERT_TRUE(q_.Start());
    ASSERT_TRUE(WaitConsumed(1));

    // 여기까지 왔으면 루프는 큐를 비우고 cv 대기로 돌아가는 중이다. 대기에
    // 실제로 들어간 뒤 Stop 하도록 짧게 양보한다 — 이 창을 놓쳐도 Stop 은
    // 안전해야 하므로 테스트가 깨지지는 않는다.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(q_.Stop());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
