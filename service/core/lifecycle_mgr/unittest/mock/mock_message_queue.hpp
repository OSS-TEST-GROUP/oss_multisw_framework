#ifndef MOCK_LIFECYCLE_MESSAGE_QUEUE_HPP_
#define MOCK_LIFECYCLE_MESSAGE_QUEUE_HPP_

#include <gmock/gmock.h>
#include "transmitter/MessageQueue.hpp"

namespace lifecycle_manager {
class MockMessageQueue : public MessageQueue {
public:
    MockMessageQueue(DDSTransmitter* transmitter) : MessageQueue(transmitter) {}
    ~MockMessageQueue() override = default;

    // Only mock virtual methods
    MOCK_METHOD(void, SendMessage, (MsgType& message), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, PublishingLoop, (), ());
};

inline MockMessageQueue* Mock_MessageQueue = nullptr;

MessageQueue::MessageQueue(DDSTransmitter* transmitter){
    transmitter_ = transmitter;
}

MessageQueue::~MessageQueue(){

}

void MessageQueue::SendMessage(MsgType& message){
    Mock_MessageQueue->SendMessage(message);
}

bool MessageQueue::Start(){
    return Mock_MessageQueue->Start();  
}

void MessageQueue::Stop(){
    Mock_MessageQueue->Stop();
}

void MessageQueue::PublishingLoop(){
    Mock_MessageQueue->PublishingLoop();
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MESSAGE_QUEUE_HPP_
