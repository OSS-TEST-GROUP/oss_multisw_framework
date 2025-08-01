#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MESSAGE_QUEUE_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MESSAGE_QUEUE_HPP_

#include "transmitter/MessageQueue.hpp"
#include <gmock/gmock.h>

namespace ros2 {
namespace adsw_manager {

class MockMessageQueue : public MessageQueue {
public:
    MOCK_METHOD(void, SendMessage, (MsgType& message), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, PublishingLoop, (), ());
};


MockMessageQueue* Mock_MessageQueue;

MessageQueue::MessageQueue() {

}
MessageQueue::~MessageQueue() {}

void MessageQueue::SendMessage(MsgType& message) {
    Mock_MessageQueue->SendMessage(message);
}
bool MessageQueue::Start() {
    return Mock_MessageQueue->Start();
}
void MessageQueue::Stop() {
    Mock_MessageQueue->Stop();
}
void MessageQueue::PublishingLoop() {
    Mock_MessageQueue->PublishingLoop();
}


}
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_UNITTEST_MOCK_MESSAGE_QUEUE_HPP_
