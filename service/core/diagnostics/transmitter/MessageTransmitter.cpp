#include "MessageTransmitter.hpp"
#include "dds/DDSProtocol.hpp"

namespace diagnostic_manager {

SubscriberInfo::SubscriberInfo() {}
SubscriberInfo::SubscriberInfo(uint64_t id, const std::string& name, ReceiveMessageCreator creator, TaskID task_id)
        : id(id), name(name), message_creator(creator), executor_task_id(task_id) {
    SubscriberInfo();
}

MessageTransmitter::MessageTransmitter(std::shared_ptr<ServiceExecutor> service_executor) : service_executor_(service_executor) {}
MessageTransmitter::~MessageTransmitter() {}

bool MessageTransmitter::Initialize() {
    // Create protocols
    protocols_[MessageProtocolType::DDS] = std::make_shared<DDSProtocol>();

    for (auto& iter : protocols_) {
        iter.second->SetForwardCallback(std::bind(&MessageTransmitter::ForwardDeliveredMessage, this, std::placeholders::_1));
        iter.second->Initialize();

        subscription_list_[iter.second->GetProtocolType()].clear();
    }

    return true;
}

void MessageTransmitter::Finalize() {
    for (auto& iter : protocols_) {
        iter.second->Finalize();
    }
}

int64_t MessageTransmitter::AddSubscriber(const std::string& name, ExecutorFunctionType func, ReceiveMessageCreator creator) {
    std::shared_ptr<ReceivedMessage> received_messaged = creator();
    TaskID task_id = service_executor_->AddTask(name, std::move(func), *received_messaged);
    if (task_id < 0) {
        std::cerr << "Failed to create the executor task. name=" << name << std::endl;
        return -1;
    }

    int64_t subscriber_id;
    {
        std::lock_guard<std::mutex> lock(subscriber_handling_mutex_);

        subscriber_id = next_subscriber_id_.fetch_add(1);
        SubscriberInfo subscriber(subscriber_id, name, creator, task_id);
        subscriber_list_[subscriber_id] = std::move(subscriber);
    }

    return subscriber_id;
}

void MessageTransmitter::RemoveSubscriber(uint64_t subscriber_id) {
    {
        std::lock_guard<std::mutex> lock(subscriber_handling_mutex_);

        auto subscriber = subscriber_list_.find(subscriber_id);
        if (subscriber == subscriber_list_.end()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(subscription_handling_mutex_);
            CleanSubscriptionID(subscriber_id);
        }
        service_executor_->RemoveTask(subscriber->second.executor_task_id);

        subscriber_list_.erase(subscriber);
    }
}

bool MessageTransmitter::PublishMessage(std::shared_ptr<Message> msg) {
    if (msg == nullptr) {
        return false;
    }

    auto protocol = protocols_.find(msg->GetProtocolType());
    if (protocol == protocols_.end()) {
        std::cerr << "The protocol type[" << (uint32_t)msg->GetProtocolType() << "] is not existed. msg_name=" << msg->GetName() << std::endl;
        return false;
    }

    return protocol->second->PublishMessage(msg);
}

bool MessageTransmitter::SubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type) {
    if (protocols_.count(type) == 0) {
        std::cerr << "Not supported protocol. protocol=" << (uint32_t)type << std::endl;
        return false;
    }

    bool need_subscribe = false;
    {
        std::lock_guard<std::mutex> subscriber_lock(subscriber_handling_mutex_);

        if (subscriber_list_.count(subscriber_id) == 0) {
            std::cerr << "Not registered subscriber." << std::endl;
            return false;
        }

        {
            std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);
            if (!HasAnySubscriptionID(name, type)) {
                need_subscribe = true;
            }

            AddSubscriptionID(name, subscriber_id, type);
        }
    }

    if (need_subscribe) {
        bool subscribe_result = protocols_[type]->SubscribeMessage(name);

        if (!subscribe_result) {
            std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);
            RemoveSubscriptionID(name, subscriber_id, type);
            return false;
        }
    }

    return true;
}

bool MessageTransmitter::UnsubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type) {
    if (protocols_.count(type) == 0) {
        std::cerr << "Not supported protocol. protocol=" << (uint32_t)type << std::endl;
        return false;
    }

    bool need_unsubscribe = false;
    {
        std::lock_guard<std::mutex> subscriber_lock(subscriber_handling_mutex_);

        if (subscriber_list_.count(subscriber_id) == 0) {
            std::cerr << "Not registered subscriber." << std::endl;
            return false;
        }

        {
            std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);
            RemoveSubscriptionID(name, subscriber_id, type);

            if (!HasAnySubscriptionID(name, type)) {
                need_unsubscribe = true;
            }
        }
    }

    if (need_unsubscribe) {
        protocols_[type]->UnsubscribeMessage(name);
    }

    return true;
}

bool MessageTransmitter::IsSubscribed(const std::string& name, MessageProtocolType type) {
    std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);

    if (subscription_list_.count(type) == 0) {
        return false;
    }
    return subscription_list_[type].count(name) > 0;
}

std::vector<std::string> MessageTransmitter::GetSubscribedMessageList() {
    std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);

    std::vector<std::string> message_list;
    for (auto& iter_protocol : subscription_list_) {
        for (auto& iter_msg : iter_protocol.second) {
            message_list.push_back(iter_msg.first);
        }
    }
    return message_list;
}

uint64_t MessageTransmitter::GetSubscribeMessageNum() {
    std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);

    uint64_t msg_num = 0;
    for (auto& iter : subscription_list_) {
        msg_num += iter.second.size();
    }
    return msg_num;
}

void MessageTransmitter::ForwardDeliveredMessage(std::shared_ptr<Message> msg) {
    std::set<int64_t> subscriber_list;

    {
        std::lock_guard<std::mutex> subscription_lock(subscription_handling_mutex_);
        subscriber_list = GetSubscriptionIDList(msg->GetName(), msg->GetProtocolType());
    }

    {
        std::lock_guard<std::mutex> subscriber_lock(subscriber_handling_mutex_);
        for (auto& iter : subscriber_list) {
            SubscriberInfo& subscriber = subscriber_list_[iter];
            auto request_task = subscriber.message_creator();
            request_task->SetMessage(msg);

            service_executor_->RequestTask(request_task);
        }
    }
}

void MessageTransmitter::AddSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type) {
    subscription_list_[type][name].insert(subscriber_id);
}

void MessageTransmitter::RemoveSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type) {
    if (subscription_list_.count(type) == 0) {
        return;
    }
    if (subscription_list_[type].count(name) == 0) {
        return;
    }
    subscription_list_[type][name].erase(subscriber_id);
    if (subscription_list_[type][name].size() == 0) {
        subscription_list_[type].erase(name);
    }
}

bool MessageTransmitter::HasSubscriptionID(const std::string& name, uint64_t subscriber_id, MessageProtocolType type) {
    if (subscription_list_.count(type) == 0) {
        return false;
    }
    if (subscription_list_[type].count(name) == 0) {
        return false;
    }
    return subscription_list_[type][name].count(subscriber_id) > 0;
}

bool MessageTransmitter::HasAnySubscriptionID(const std::string& name, MessageProtocolType type) {
    if (subscription_list_.count(type) == 0) {
        return false;
    }
    return subscription_list_[type].count(name) > 0;
}

std::set<int64_t> MessageTransmitter::GetSubscriptionIDList(const std::string& name, MessageProtocolType type) {
    std::set<int64_t> subscription_list;
    if (subscription_list_.count(type) > 0) {
        if (subscription_list_[type].count(name) > 0) {
            subscription_list = subscription_list_[type][name];
        }
    }
    return subscription_list;
}

void MessageTransmitter::CleanSubscriptionID(uint64_t subscriber_id) {
    for (auto& iter_protocol : subscription_list_) {
        for (auto& iter_name : iter_protocol.second) {
            iter_name.second.erase(subscriber_id);
        }
    }
}

} // namespace diagnostic_manager
