// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "transmitter/MessageQueue.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "mock/MockADSWManager.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockLogger.hpp"

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

class MessageQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        Mock_DDSTransmitter = new MockDDSTransmitter();

        // Note: Get() is not mocked - it returns shared_instance_.get() directly
        ON_CALL(*Mock_ADSWManager, GetTopic(::testing::_)).WillByDefault(::testing::Return("mock_topic"));
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(::testing::Return(std::make_shared<MockDDSTransmitter>()));
    }

    void TearDown() override {
        delete Mock_Logger;
        Mock_Logger = nullptr;
        
        // Reset the static shared_ptr - this will delete Mock_ADSWManager
        ADSWManager::SetShared(nullptr);
        Mock_ADSWManager = nullptr;

        delete Mock_DDSTransmitter;
        Mock_DDSTransmitter = nullptr;
    }
    ros2::adsw_manager::MessageQueue queue;
};

TEST_F(MessageQueueTest, ConstructionAndDestruction) {
    // Just test that it constructs and destructs without crashing
}

TEST_F(MessageQueueTest, StartAndStop) {
    ASSERT_TRUE(queue.Start());
    queue.Stop();
}

TEST_F(MessageQueueTest, SendMessageEnqueues) {
    ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x1234)};
    queue.SendMessage(msg);
    // Can't check queue directly, but no crash means success for basic test
}

TEST_F(MessageQueueTest, PublishingLoopProcessesMessages) {
    ASSERT_TRUE(queue.Start());

    ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(0x5678)};
    queue.SendMessage(msg);
    
    // Allow some time for the publishing loop to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    queue.Stop();
}
TEST_F(MessageQueueTest, StopStopsPublishingLoop) {
    ASSERT_TRUE(queue.Start());
    queue.Stop();
    // No crash means stop worked
}
TEST_F(MessageQueueTest, MultipleMessages) {
    ASSERT_TRUE(queue.Start());
    for (int i = 0; i < 10; ++i) {
        ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::UPDATE_SERVICE_LIST_RESPONSE, reinterpret_cast<void*>(i)};
        queue.SendMessage(msg);
    }
    // Allow some time for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.Stop();

    // No crash means stop worked
    ASSERT_NO_THROW(queue.Stop());
}

TEST_F(MessageQueueTest, SendMessage_BeforeStart_TC007) {
    ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x1111)};
    EXPECT_NO_THROW(queue.SendMessage(msg));
}

TEST_F(MessageQueueTest, SendMessage_AfterStop_TC008) {
    ASSERT_TRUE(queue.Start());
    queue.Stop();
    
    ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(0x2222)};
    EXPECT_NO_THROW(queue.SendMessage(msg));
}

TEST_F(MessageQueueTest, StartMultipleTimes_TC009) {
    ASSERT_TRUE(queue.Start());
    
    // Starting again shouldn't crash (though may create multiple threads)
    EXPECT_NO_THROW(queue.Start());
    
    queue.Stop();
}

TEST_F(MessageQueueTest, StopMultipleTimes_TC010) {
    ASSERT_TRUE(queue.Start());
    queue.Stop();
    
    // Stopping again should be safe
    EXPECT_NO_THROW(queue.Stop());
}

TEST_F(MessageQueueTest, AllMessageTypes_TC011) {
    ASSERT_TRUE(queue.Start());
    
    ros2::adsw_manager::MsgType msg1{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, nullptr};
    ros2::adsw_manager::MsgType msg2{ros2::adsw_manager::MessageType::START_SERVICE_RESPONSE, nullptr};
    ros2::adsw_manager::MsgType msg3{ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST, nullptr};
    ros2::adsw_manager::MsgType msg4{ros2::adsw_manager::MessageType::STOP_SERVICE_RESPONSE, nullptr};
    ros2::adsw_manager::MsgType msg5{ros2::adsw_manager::MessageType::UPDATE_SERVICE_LIST_REQUEST, nullptr};
    ros2::adsw_manager::MsgType msg6{ros2::adsw_manager::MessageType::UPDATE_SERVICE_LIST_RESPONSE, nullptr};
    
    queue.SendMessage(msg1);
    queue.SendMessage(msg2);
    queue.SendMessage(msg3);
    queue.SendMessage(msg4);
    queue.SendMessage(msg5);
    queue.SendMessage(msg6);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.Stop();
}

TEST_F(MessageQueueTest, NullPointerMessage_TC012) {
    ASSERT_TRUE(queue.Start());
    
    ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, nullptr};
    EXPECT_NO_THROW(queue.SendMessage(msg));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Stop();
}

TEST_F(MessageQueueTest, RapidMessaging_TC013) {
    ASSERT_TRUE(queue.Start());
    
    for (int i = 0; i < 100; ++i) {
        ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, 
                                         reinterpret_cast<void*>(static_cast<uintptr_t>(i))};
        queue.SendMessage(msg);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    queue.Stop();
}

TEST_F(MessageQueueTest, EmptyQueueStop_TC014) {
    ASSERT_TRUE(queue.Start());
    
    // Stop without sending any messages
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(queue.Stop());
}

TEST_F(MessageQueueTest, InterleavedSendAndStop_TC015) {
    ASSERT_TRUE(queue.Start());
    
    ros2::adsw_manager::MsgType msg1{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, nullptr};
    queue.SendMessage(msg1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    ros2::adsw_manager::MsgType msg2{ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST, nullptr};
    queue.SendMessage(msg2);
    
    queue.Stop();
}

TEST_F(MessageQueueTest, ConcurrentSendMessages_TC016) {
    ASSERT_TRUE(queue.Start());
    
    std::thread t1([this]() {
        for (int i = 0; i < 50; ++i) {
            ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::START_SERVICE_REQUEST, 
                                             reinterpret_cast<void*>(static_cast<uintptr_t>(i))};
            queue.SendMessage(msg);
        }
    });
    
    std::thread t2([this]() {
        for (int i = 50; i < 100; ++i) {
            ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::STOP_SERVICE_REQUEST, 
                                             reinterpret_cast<void*>(static_cast<uintptr_t>(i))};
            queue.SendMessage(msg);
        }
    });
    
    t1.join();
    t2.join();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    queue.Stop();
}

TEST_F(MessageQueueTest, StopDuringMessageProcessing_TC017) {
    ASSERT_TRUE(queue.Start());
    
    // Send a bunch of messages
    for (int i = 0; i < 50; ++i) {
        ros2::adsw_manager::MsgType msg{ros2::adsw_manager::MessageType::UPDATE_SERVICE_LIST_REQUEST, 
                                         reinterpret_cast<void*>(static_cast<uintptr_t>(i))};
        queue.SendMessage(msg);
    }
    
    // Stop immediately without waiting
    EXPECT_NO_THROW(queue.Stop());
}

TEST_F(MessageQueueTest, ConstructMultipleQueues_TC018) {
    ros2::adsw_manager::MessageQueue queue1;
    ros2::adsw_manager::MessageQueue queue2;
    ros2::adsw_manager::MessageQueue queue3;
    
    ASSERT_TRUE(queue1.Start());
    ASSERT_TRUE(queue2.Start());
    ASSERT_TRUE(queue3.Start());
    
    queue1.Stop();
    queue2.Stop();
    queue3.Stop();
}


// ═══════════════════════════════════════════════════════════════════════════
// PublishingLoop 의 실제 발행 경로
//
// 왜 미커버였나: 위 픽스처의 GetTransmitter() 는 호출할 때마다 *새* 목을
// 돌려주고(std::make_shared<MockDDSTransmitter>()), 그 목에는
// GetMessageEndpoint 기본값이 없어 언제나 nullptr 이 나온다. 그래서
// `if (endpoint != nullptr)` 안쪽 — 큐가 존재하는 유일한 이유인 Write 호출 —
// 이 실행 0회였다. 위 테스트들이 전부 "크래시 안 나면 통과"인 것도 그래서다.
//
// 무엇을 지키는가: 소스 주석에 남아 있는 사고 그대로다. 예전에는 Write 의
// bool 반환을 RETCODE_OK(=0) 과 비교해서 성공한 발행을 전부 "Failed" 로
// 찍었다. 성공/실패 판정과, 실패해도 루프가 멈추지 않고 다음 메시지로
// 넘어가는지를 고정한다 — 여기서 멈추면 큐가 막혀 이후 발행이 전부 사라진다.
// ═══════════════════════════════════════════════════════════════════════════

class MessageQueueWriteTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Mock_Logger = new NiceMock<MockLogger>();
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));

        Mock_ADSWManager = new NiceMock<MockADSWManager>();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        transmitter_ = std::make_shared<NiceMock<MockDDSTransmitter>>();
        endpoint_ = std::make_shared<NiceMock<MockDDSMessageEndpoint>>();
        Mock_DDSTransmitter = transmitter_.get();
        Mock_DDSMessageEndpoint = endpoint_.get();

        ON_CALL(*Mock_ADSWManager, GetTopic(_)).WillByDefault(Return("mmf/test/topic"));
        // 매번 새 목을 주면 GetMessageEndpoint 를 프로그래밍할 수 없다 —
        // 같은 인스턴스를 계속 돌려줘야 한다.
        ON_CALL(*Mock_ADSWManager, GetTransmitter()).WillByDefault(Return(transmitter_));
        ON_CALL(*transmitter_, GetMessageEndpoint(_)).WillByDefault(Return(endpoint_));
        ON_CALL(*endpoint_, Write(_)).WillByDefault(::testing::Invoke([this](void* p) {
            {
                std::lock_guard<std::mutex> lk(m_);
                written_.push_back(p);
            }
            cv_.notify_all();
            return write_ok_.load();
        }));
    }

    void TearDown() override {
        Mock_DDSMessageEndpoint = nullptr;
        Mock_DDSTransmitter = nullptr;
        endpoint_.reset();
        transmitter_.reset();
        ADSWManager::SetShared(nullptr);   // Mock_ADSWManager 를 함께 지운다
        Mock_ADSWManager = nullptr;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    // 발행 스레드가 n 건을 쓸 때까지 기다린다. sleep 고정값 대신 조건 대기 —
    // 느린 CI 에서 간헐 실패하지 않게.
    bool WaitForWrites(size_t n, int ms = 3000) {
        std::unique_lock<std::mutex> lk(m_);
        return cv_.wait_for(lk, std::chrono::milliseconds(ms),
                            [&] { return written_.size() >= n; });
    }

    std::shared_ptr<NiceMock<MockDDSTransmitter>> transmitter_;
    std::shared_ptr<NiceMock<MockDDSMessageEndpoint>> endpoint_;
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<void*> written_;
    std::atomic<bool> write_ok_{true};
};

TEST_F(MessageQueueWriteTest, WritesQueuedMessageToItsEndpoint) {
    MessageQueue q;
    ASSERT_TRUE(q.Start());

    void* payload = reinterpret_cast<void*>(0xBEEF);
    MsgType m1{MessageType::START_SERVICE_REQUEST, payload};
    q.SendMessage(m1);

    ASSERT_TRUE(WaitForWrites(1)) << "발행 스레드가 Write 까지 가지 못했다";
    {
        std::lock_guard<std::mutex> lk(m_);
        // 큐는 포인터를 그대로 넘겨야 한다. 복사하거나 바꾸면 호출부가
        // 소유한 멤버 버퍼와 어긋난다.
        EXPECT_EQ(written_[0], payload);
    }
    q.Stop();
}

TEST_F(MessageQueueWriteTest, LooksUpEndpointByTheMessagesTopic) {
    // 토픽은 ADSWManager::GetTopic(msg.first) 로 정한다. 여기가 어긋나면
    // 엉뚱한 엔드포인트로 나가거나 아무 데도 안 나간다.
    EXPECT_CALL(*Mock_ADSWManager, GetTopic(MessageType::STOP_SERVICE_REQUEST))
        .WillRepeatedly(Return("mmf/test/stop"));
    EXPECT_CALL(*transmitter_, GetMessageEndpoint(std::string("mmf/test/stop")))
        .WillRepeatedly(Return(endpoint_));

    MessageQueue q;
    ASSERT_TRUE(q.Start());
    MsgType m2{MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(0x11)};
    q.SendMessage(m2);

    EXPECT_TRUE(WaitForWrites(1));
    q.Stop();
}

TEST_F(MessageQueueWriteTest, FailedWriteDoesNotStallTheQueue) {
    // Write 가 false 를 내도 그 메시지는 버리고 다음으로 가야 한다.
    // 여기서 멈추면 큐가 막혀 이후 발행이 전부 사라진다.
    write_ok_ = false;

    MessageQueue q;
    ASSERT_TRUE(q.Start());
    MsgType m3{MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x1)};
    q.SendMessage(m3);
    MsgType m4{MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x2)};
    q.SendMessage(m4);
    MsgType m5{MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x3)};
    q.SendMessage(m5);

    EXPECT_TRUE(WaitForWrites(3)) << "실패 한 건에 루프가 멈췄다";
    q.Stop();
}

TEST_F(MessageQueueWriteTest, SuccessAndFailureBothDrainTheQueue) {
    // 성공/실패가 섞여도 전부 소진돼야 한다. (예전엔 bool 을 RETCODE_OK 와
    // 비교해 성공을 실패로 판정했다 — 그때도 소진은 됐지만 로그가 거짓이었다.)
    MessageQueue q;
    ASSERT_TRUE(q.Start());
    MsgType m6{MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0xA)};
    q.SendMessage(m6);
    ASSERT_TRUE(WaitForWrites(1));

    write_ok_ = false;
    MsgType m7{MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(0xB)};
    q.SendMessage(m7);
    ASSERT_TRUE(WaitForWrites(2));

    write_ok_ = true;
    MsgType m8{MessageType::UPDATE_SERVICE_LIST_REQUEST, reinterpret_cast<void*>(0xC)};
    q.SendMessage(m8);
    ASSERT_TRUE(WaitForWrites(3));

    {
        std::lock_guard<std::mutex> lk(m_);
        EXPECT_EQ(written_, (std::vector<void*>{reinterpret_cast<void*>(0xA),
                                                reinterpret_cast<void*>(0xB),
                                                reinterpret_cast<void*>(0xC)}));
    }
    q.Stop();
}

TEST_F(MessageQueueWriteTest, MissingEndpointIsSkippedWithoutWriting) {
    // 아직 등록되지 않은 토픽으로 보내면 조용히 넘어가야 한다 — 기동 순서상
    // 엔드포인트보다 발행이 먼저 오는 창이 실제로 있다.
    ON_CALL(*transmitter_, GetMessageEndpoint(_))
        .WillByDefault(Return(std::shared_ptr<DDSMessageEndpoint>{}));
    EXPECT_CALL(*endpoint_, Write(_)).Times(0);

    MessageQueue q;
    ASSERT_TRUE(q.Start());
    MsgType m9{MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(0x99)};
    q.SendMessage(m9);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_NO_THROW(q.Stop());

    std::lock_guard<std::mutex> lk(m_);
    EXPECT_TRUE(written_.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
