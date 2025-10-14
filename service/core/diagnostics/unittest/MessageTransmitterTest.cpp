#include <gtest/gtest.h>

#include "transmitter/MessageTransmitter.cpp"
#include "transmitter/MessageProtocol.cpp"
#include "transmitter/request_msg/IncomingMessageBase.cpp"
#include "transmitter/Message.cpp"
#include "diagnostic/VariantData.cpp"

#include "mock/DDSProtocol_mock.hpp"
#include "mock/MessageProtocol_mock.hpp"
#include "mock/ServiceExecutor_mock.hpp"
#include "mock/RequestBase_mock.hpp"
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
using ::testing::Invoke;

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class MessageTransmitterTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        service_executor_mock_ = std::make_shared<ServiceExecutorMock>();
        Mock_ServiceExecutorMock = service_executor_mock_.get();
        message_protocol_mock_ = std::make_shared<MessageProtocolMock>();
        Mock_MessageProtocolMock = message_protocol_mock_.get();
        Mock_DDSProtocol = new DDSProtocolMock;

        ins = new MessageTransmitter(service_executor_mock_);
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        delete ins;

        service_executor_mock_.reset();
        Mock_ServiceExecutorMock = nullptr;
        message_protocol_mock_.reset();
        Mock_MessageProtocolMock = nullptr;
        delete Mock_DDSProtocol;
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    MessageTransmitter* ins;

    std::shared_ptr<ServiceExecutorMock> service_executor_mock_;
    std::shared_ptr<MessageProtocolMock> message_protocol_mock_;
};

TEST_F(MessageTransmitterTest, Initialize_and_Finalize_TC001) {
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());
    EXPECT_EQ(1, ins->protocols_.size());
    ins->Finalize();
}

TEST_F(MessageTransmitterTest, AddSubscriber_TC001) {
    auto receive_func = [](std::shared_ptr<RequestBase> request) {
    };
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };

    EXPECT_EQ(0, ins->subscriber_list_.size());

    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask("HappySubscriber", _, _)).WillOnce(Return(1234));
    
    ins->AddSubscriber("HappySubscriber", receive_func, message_creator);
    EXPECT_EQ(1, ins->subscriber_list_.size());
    EXPECT_EQ(1, ins->subscriber_list_[1].id);
    EXPECT_EQ("HappySubscriber", ins->subscriber_list_[1].name);
    EXPECT_TRUE(ins->subscriber_list_[1].message_creator);
    EXPECT_EQ(1234, ins->subscriber_list_[1].executor_task_id);
}

TEST_F(MessageTransmitterTest, RemoveSubscriber_TC001) {
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };
    ins->subscriber_list_[1] = std::move(SubscriberInfo(1, "Hello", message_creator, 1234));

    EXPECT_CALL(*Mock_ServiceExecutorMock, RemoveTask(1234)).WillOnce(Return());

    ins->RemoveSubscriber(1);
    EXPECT_EQ(0, ins->subscriber_list_.size());
}

TEST_F(MessageTransmitterTest, PublishMessage_TC001) {
    std::shared_ptr<Message> test_msg = std::make_shared<Message>("Test", MessageProtocolType::DDS, "TestType", nullptr);

    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));
    EXPECT_CALL(*Mock_DDSProtocol, PublishMessage(test_msg)).WillOnce(Return(true));

    EXPECT_TRUE(ins->Initialize());

    ins->PublishMessage(test_msg);

    ins->Finalize();
}

TEST_F(MessageTransmitterTest, SubscribeMessage_TC001) {
    // Prepare for the test
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());

    auto receive_func = [](std::shared_ptr<RequestBase> request) {
    };
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };

    EXPECT_EQ(0, ins->subscriber_list_.size());

    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask("HappySubscriber", _, _)).WillOnce(Return(1234));
    
    int64_t subscriber_id = ins->AddSubscriber("HappySubscriber", receive_func, message_creator);
    EXPECT_TRUE(subscriber_id > 0);

    // Test
    EXPECT_CALL(*Mock_DDSProtocol, SubscribeMessage("HelloTopic")).WillOnce(Return(true));

    EXPECT_TRUE(ins->SubscribeMessage(subscriber_id, "HelloTopic", MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("HelloTopic", subscriber_id, MessageProtocolType::DDS));

    ins->Finalize();
}

TEST_F(MessageTransmitterTest, SubscribeMessage_TC002_FailedSubscribe) {
    // Prepare for the test
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());

    auto receive_func = [](std::shared_ptr<RequestBase> request) {
    };
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };

    EXPECT_EQ(0, ins->subscriber_list_.size());

    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask("HappySubscriber", _, _)).WillOnce(Return(1234));
    
    int64_t subscriber_id = ins->AddSubscriber("HappySubscriber", receive_func, message_creator);
    EXPECT_TRUE(subscriber_id > 0);

    // Test
    EXPECT_CALL(*Mock_DDSProtocol, SubscribeMessage("HelloTopic")).WillOnce(Return(false));

    EXPECT_FALSE(ins->SubscribeMessage(subscriber_id, "HelloTopic", MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("HelloTopic", subscriber_id, MessageProtocolType::DDS));

    ins->Finalize();
}

TEST_F(MessageTransmitterTest, UnsubscribeMessage_TC001) {
    // Prepare for the test
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());

    auto receive_func = [](std::shared_ptr<RequestBase> request) {
    };
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };

    EXPECT_EQ(0, ins->subscriber_list_.size());

    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask("HappySubscriber", _, _)).WillOnce(Return(1234));
    
    int64_t subscriber_id = ins->AddSubscriber("HappySubscriber", receive_func, message_creator);
    EXPECT_TRUE(subscriber_id > 0);

    EXPECT_CALL(*Mock_DDSProtocol, SubscribeMessage("HelloTopic")).WillOnce(Return(true));

    EXPECT_TRUE(ins->SubscribeMessage(subscriber_id, "HelloTopic", MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("HelloTopic", subscriber_id, MessageProtocolType::DDS));

    // Test
    EXPECT_CALL(*Mock_DDSProtocol, UnsubscribeMessage("HelloTopic")).WillOnce(Return(true));

    EXPECT_TRUE(ins->UnsubscribeMessage(subscriber_id, "HelloTopic", MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("HelloTopic", subscriber_id, MessageProtocolType::DDS));

    ins->Finalize();
}

TEST_F(MessageTransmitterTest, IsSubscribed_TC001) {
    ins->AddSubscriptionID("Hello", 123, MessageProtocolType::DDS);

    EXPECT_TRUE(ins->IsSubscribed("Hello", MessageProtocolType::DDS));
    EXPECT_FALSE(ins->IsSubscribed("Hello2", MessageProtocolType::DDS));
}

TEST_F(MessageTransmitterTest, GetSubscribedMessageList_TC001) {
    ins->AddSubscriptionID("Hello", 123, MessageProtocolType::DDS);
    ins->AddSubscriptionID("Hello2", 123, MessageProtocolType::DDS);
    ins->AddSubscriptionID("Hello5", 123, MessageProtocolType::DDS);

    auto list = ins->GetSubscribedMessageList();
    EXPECT_EQ(3, list.size());
    EXPECT_TRUE(std::find(list.begin(), list.end(), "Hello") != list.end());
    EXPECT_TRUE(std::find(list.begin(), list.end(), "Hello2") != list.end());
    EXPECT_TRUE(std::find(list.begin(), list.end(), "Hello5") != list.end());
}

TEST_F(MessageTransmitterTest, GetSubscribeMessageNum_TC001) {
    ins->AddSubscriptionID("Hello", 123, MessageProtocolType::DDS);
    ins->AddSubscriptionID("Hello2", 123, MessageProtocolType::DDS);
    ins->AddSubscriptionID("Hello5", 123, MessageProtocolType::DDS);

    EXPECT_EQ(3, ins->GetSubscribeMessageNum());
}

TEST_F(MessageTransmitterTest, ForwardDeliveredMessage_TC001) {
    // Prepare for the test
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillOnce(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());

    auto receive_func = [](std::shared_ptr<RequestBase> request) {
    };
    auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
        return std::make_shared<IncomingMessageBase>();
    };

    EXPECT_EQ(0, ins->subscriber_list_.size());

    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask("HappySubscriber", _, _)).WillOnce(Return(1234));
    
    int64_t subscriber_id = ins->AddSubscriber("HappySubscriber", receive_func, message_creator);
    EXPECT_TRUE(subscriber_id > 0);

    EXPECT_CALL(*Mock_DDSProtocol, SubscribeMessage("HelloTopic")).WillOnce(Return(true));

    EXPECT_TRUE(ins->SubscribeMessage(subscriber_id, "HelloTopic", MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("HelloTopic", subscriber_id, MessageProtocolType::DDS));

    // Test
    std::shared_ptr<Message> test_msg = std::make_shared<Message>("HelloTopic", MessageProtocolType::DDS, "TestType", nullptr);
    EXPECT_CALL(*Mock_ServiceExecutorMock, RequestTask(_)).WillOnce(Return(true));
    ins->ForwardDeliveredMessage(test_msg);

    ins->Finalize();
}

TEST_F(MessageTransmitterTest, SubscriptionIDHandling_TC001) {
    EXPECT_FALSE(ins->HasSubscriptionID("A", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasAnySubscriptionID("A", MessageProtocolType::DDS));

    ins->AddSubscriptionID("A", 123, MessageProtocolType::DDS);
    EXPECT_TRUE(ins->HasSubscriptionID("A", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("A", 1234, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("B", 123, MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasAnySubscriptionID("A", MessageProtocolType::DDS));

    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));

    ins->AddSubscriptionID("B", 123, MessageProtocolType::DDS);
    EXPECT_TRUE(ins->HasSubscriptionID("B", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("B", 1234, MessageProtocolType::DDS));
    
    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));

    ins->AddSubscriptionID("A", 1234, MessageProtocolType::DDS);
    EXPECT_TRUE(ins->HasSubscriptionID("A", 1234, MessageProtocolType::DDS));
    
    EXPECT_EQ((std::set<int64_t>{123, 1234}), ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));

    ins->RemoveSubscriptionID("A", 123, MessageProtocolType::DDS);
    EXPECT_FALSE(ins->HasSubscriptionID("A", 123, MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("A", 1234, MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("B", 123, MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasAnySubscriptionID("A", MessageProtocolType::DDS));

    EXPECT_EQ(std::set<int64_t>{1234}, ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));

    ins->RemoveSubscriptionID("A", 1234, MessageProtocolType::DDS);
    EXPECT_FALSE(ins->HasSubscriptionID("A", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("A", 1234, MessageProtocolType::DDS));
    EXPECT_TRUE(ins->HasSubscriptionID("B", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasAnySubscriptionID("A", MessageProtocolType::DDS));

    EXPECT_EQ(std::set<int64_t>{}, ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{123}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));

    ins->CleanSubscriptionID(123);
    EXPECT_FALSE(ins->HasSubscriptionID("A", 123, MessageProtocolType::DDS));
    EXPECT_FALSE(ins->HasSubscriptionID("B", 123, MessageProtocolType::DDS));

    EXPECT_EQ(std::set<int64_t>{}, ins->GetSubscriptionIDList("A", MessageProtocolType::DDS));
    EXPECT_EQ(std::set<int64_t>{}, ins->GetSubscriptionIDList("B", MessageProtocolType::DDS));
}

TEST_F(MessageTransmitterTest, MultiThreadTest_TC001) {
    constexpr uint32_t TEST_THREAD = 20;
    constexpr uint32_t TEST_COUNT = 1000;
    std::atomic<uint64_t> call_count(0U);
    std::atomic<uint64_t> called_count(0U);

    // Prepare for the test
    EXPECT_CALL(*Mock_DDSProtocol, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, Finalize()).WillOnce(Return());
    EXPECT_CALL(*Mock_DDSProtocol, GetProtocolType()).WillRepeatedly(Return(MessageProtocolType::DDS));

    EXPECT_TRUE(ins->Initialize());
    
    std::atomic<uint64_t> next_task_id(1U);
    EXPECT_CALL(*Mock_ServiceExecutorMock, AddTask(_, _, _)).WillRepeatedly(
        Invoke([&](const std::string& name, ExecutorFunctionType type, const RequestBase& req){
            return next_task_id.fetch_add(1);
        }));
    EXPECT_CALL(*Mock_DDSProtocol, SubscribeMessage(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_DDSProtocol, UnsubscribeMessage(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*Mock_ServiceExecutorMock, RequestTask(_)).WillRepeatedly(Invoke([&](std::shared_ptr<RequestBase> task){ called_count.fetch_add(1); return true; }));

    auto test_thread_func = [&](int32_t idx) {
        auto receive_func = [](std::shared_ptr<RequestBase> request) {
        };
        auto message_creator = []() -> std::shared_ptr<IncomingMessageBase> {
            return std::make_shared<IncomingMessageBase>();
        };
        std::string subscriber_name = "HappySubscriber"; subscriber_name += idx;
        std::string subscription_name = "HelloTopic"; subscription_name += idx;

        for (uint32_t test_count = 0; test_count < TEST_COUNT; test_count++) {
            int64_t subscriber_id = ins->AddSubscriber(subscriber_name, receive_func, message_creator);
            EXPECT_TRUE(subscriber_id > 0);

            EXPECT_TRUE(ins->SubscribeMessage(subscriber_id, subscription_name, MessageProtocolType::DDS));
            { // To prevent race condition because of using private method directly here
                std::lock_guard<std::mutex> subscription_lock(ins->subscription_handling_mutex_);
                EXPECT_TRUE(ins->HasSubscriptionID(subscription_name, subscriber_id, MessageProtocolType::DDS));
            }

            // Test
            std::shared_ptr<Message> test_msg = std::make_shared<Message>(subscription_name, MessageProtocolType::DDS, "TestType", nullptr);
            ins->ForwardDeliveredMessage(test_msg);
            call_count.fetch_add(1);

            EXPECT_TRUE(ins->UnsubscribeMessage(subscriber_id, subscription_name, MessageProtocolType::DDS));
            { // To prevent race condition because of using private method directly here
                std::lock_guard<std::mutex> subscription_lock(ins->subscription_handling_mutex_);
                EXPECT_FALSE(ins->HasSubscriptionID(subscription_name, subscriber_id, MessageProtocolType::DDS));
            }
        }
    };

    std::vector<std::thread> thread_list;
    for (uint32_t idx = 0; idx < TEST_THREAD; idx++) {
        thread_list.emplace_back(test_thread_func, idx);
    }

    for (uint32_t idx = 0; idx < TEST_THREAD; idx++) {
        thread_list[idx].join();
    }

    EXPECT_EQ(call_count, called_count);
    EXPECT_EQ(0, ins->GetSubscribeMessageNum());
    ins->Finalize();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
