// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "transmitter/MessageQueue.hpp"

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

using namespace ros2::adsw_manager;

class MessageQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        Mock_ADSWManager = new MockADSWManager();
        ADSWManager::SetShared(std::shared_ptr<ADSWManager>(Mock_ADSWManager));

        Mock_DDSTransmitter = new MockDDSTransmitter();

        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(::testing::Return(Mock_ADSWManager));
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


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
