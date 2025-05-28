#ifndef DIAGNOSTICS_TRASMITTER_MESSAGE_TRANSMITTER_HPP_
#define DIAGNOSTICS_TRASMITTER_MESSAGE_TRANSMITTER_HPP_

#include <iostream>
#include <memory>
#include <unordered_map>
#include <set>
#include <list>

#include "executor/ServiceExecutor.hpp"
#include "executor/ServiceTask.hpp"
#include "Message.hpp"
#include "MessageProtocolType.hpp"
#include "MessageProtocol.hpp"
#include "MessageTransmitterType.hpp"
#include "ReceivedMessage.hpp"

namespace diagnostic_manager {

class SubscriberInfo {
  public:
    SubscriberInfo();
    SubscriberInfo(uint64_t id, const std::string& name, ReceiveMessageCreator creator, TaskID task_id);

    int64_t id;
    std::string name;
    ReceiveMessageCreator message_creator;
    TaskID executor_task_id;
};

class MessageTransmitter {
  public:
    MessageTransmitter(std::shared_ptr<ServiceExecutor> service_executor);
    virtual ~MessageTransmitter();

    bool Initialize();
    void Finalize();

    int64_t AddSubscriber(const std::string& name, ExecutorFunctionType func, ReceiveMessageCreator creator);
    void RemoveSubscriber(uint64_t subscriber_id);

    bool PublishMessage(std::shared_ptr<Message> msg);

    bool SubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type);
    bool UnsubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type);

    bool IsSubscribed(const std::string& name, MessageProtocolType type);
    std::vector<std::string> GetSubscribedMessageList();
    uint64_t GetSubscribeMessageNum();

    void ForwardDeliveredMessage(std::shared_ptr<Message> msg);

  private:
    void AddSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type);
    void RemoveSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type);
    bool HasSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type);
    bool HasAnySubscriptionID(const std::string& name, MessageProtocolType type);

    std::set<int64_t> GetSubscriptionIDList(const std::string& name, MessageProtocolType type);

    void CleanSubscriptionID(uint64_t subscriber_id);

    std::unordered_map<MessageProtocolType, std::shared_ptr<MessageProtocol>> protocols_;

    std::atomic<int64_t> next_subscriber_id_{1};
    std::unordered_map<int64_t, SubscriberInfo> subscriber_list_;
    std::mutex subscriber_handling_mutex_;

    std::unordered_map<MessageProtocolType, std::unordered_map<std::string, std::set<int64_t>>> subscription_list_;
    std::mutex subscription_handling_mutex_;

    std::shared_ptr<ServiceExecutor> service_executor_;
};

} // namespace diagnostic_manager

#endif
