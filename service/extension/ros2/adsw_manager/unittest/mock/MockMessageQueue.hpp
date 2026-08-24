#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MESSAGE_QUEUE_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MESSAGE_QUEUE_HPP_

#include "transmitter/MessageQueue.hpp"
#include <gmock/gmock.h>

namespace ros2 {
namespace adsw_manager {

class MockMessageQueue : public MessageQueue {
public:
    MockMessageQueue() : MessageQueue() {}
     ~MockMessageQueue() override = default;

    MOCK_METHOD(void, SendMessage, (MsgType& message), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, PublishingLoop, (), ());
};


MockMessageQueue* Mock_MessageQueue;

MessageQueue::MessageQueue() {

}
// MessageQueue::~MessageQueue() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

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
