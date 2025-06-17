#ifndef DIAGNOSTICS_UNITTEST_MOCK_MESSAGE_TRANSMITTER_MOCK_HPP_
#define DIAGNOSTICS_UNITTEST_MOCK_MESSAGE_TRANSMITTER_MOCK_HPP_

#include <gmock/gmock.h>
#include "transmitter/MessageTransmitter.hpp"

namespace diagnostic_manager {

class MessageTransmitterMock {
public:
    MOCK_METHOD0(Initialize, bool());
    MOCK_METHOD0(Finalize, void());
    MOCK_METHOD3(AddSubscriber, int64_t(const std::string& name, ExecutorFunctionType func, ReceiveMessageCreator creator));
    MOCK_METHOD1(RemoveSubscriber, void(uint64_t subscriber_id));
    MOCK_METHOD1(PublishMessage, bool(std::shared_ptr<Message> msg));
    MOCK_METHOD3(SubscribeMessage, bool(uint64_t subscriber_id, const std::string& name, MessageProtocolType type));
    MOCK_METHOD3(UnsubscribeMessage, bool(uint64_t subscriber_id, const std::string& name, MessageProtocolType type));
    MOCK_METHOD2(IsSubscribed, bool(const std::string& name, MessageProtocolType type));
    MOCK_METHOD0(GetSubscribedMessageList, std::vector<std::string>());
    MOCK_METHOD0(GetSubscribeMessageNum, uint64_t());
    MOCK_METHOD1(ForwardDeliveredMessage, void(std::shared_ptr<Message> msg));
};

MessageTransmitterMock* Mock_MessageTransmitterMock;

SubscriberInfo::SubscriberInfo() {}
SubscriberInfo::SubscriberInfo(uint64_t id, const std::string& name, ReceiveMessageCreator creator, TaskID task_id) {}

MessageTransmitter::MessageTransmitter(std::shared_ptr<ServiceExecutor> service_executor) {}
MessageTransmitter::~MessageTransmitter() {}

bool MessageTransmitter::Initialize() {
    return Mock_MessageTransmitterMock->Initialize();
}

void MessageTransmitter::Finalize() {
    Mock_MessageTransmitterMock->Finalize();
}

int64_t MessageTransmitter::AddSubscriber(const std::string& name, ExecutorFunctionType func, ReceiveMessageCreator creator) {
    return Mock_MessageTransmitterMock->AddSubscriber(name, func, creator);
}

void MessageTransmitter::RemoveSubscriber(uint64_t subscriber_id) {
    Mock_MessageTransmitterMock->RemoveSubscriber(subscriber_id);
}

bool MessageTransmitter::PublishMessage(std::shared_ptr<Message> msg) {
    return Mock_MessageTransmitterMock->PublishMessage(msg);
}

bool MessageTransmitter::SubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type) {
    return Mock_MessageTransmitterMock->SubscribeMessage(subscriber_id, name, type);
}

bool MessageTransmitter::UnsubscribeMessage(uint64_t subscriber_id, const std::string& name, MessageProtocolType type) {
    return Mock_MessageTransmitterMock->UnsubscribeMessage(subscriber_id, name, type);
}

bool MessageTransmitter::IsSubscribed(const std::string& name, MessageProtocolType type) {
    return Mock_MessageTransmitterMock->IsSubscribed(name, type);
}

std::vector<std::string> MessageTransmitter::GetSubscribedMessageList() {
    return Mock_MessageTransmitterMock->GetSubscribedMessageList();
}

uint64_t MessageTransmitter::GetSubscribeMessageNum() {
    return Mock_MessageTransmitterMock->GetSubscribeMessageNum();
}

void MessageTransmitter::ForwardDeliveredMessage(std::shared_ptr<Message> msg) {
    Mock_MessageTransmitterMock->ForwardDeliveredMessage(msg);
}

}  // namespace diagnostic_manager

#endif  // DIAGNOSTICS_UNITTEST_MOCK_MESSAGE_TRANSMITTER_MOCK_HPP_
