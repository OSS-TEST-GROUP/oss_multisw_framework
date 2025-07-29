#ifndef LIFECYCLE_MESSAGE_TRANSMITTER_MESSAGE_QUEUE_HPP_
#define LIFECYCLE_MESSAGE_TRANSMITTER_MESSAGE_QUEUE_HPP_

#include <atomic>
#include <string>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "include/MessageType.hpp"

namespace lifecycle_manager {

using MsgType = std::pair<MessageType, void*>;

class DDSTransmitter;

class MessageQueue {
  public:
    MessageQueue(DDSTransmitter* transmitter);
    virtual ~MessageQueue();

    void SendMessage(MsgType& message);

    bool Start() ;
    void Stop();

    void PublishingLoop();

  private:
    DDSTransmitter* transmitter_;

    std::atomic<bool> stop_publisher_{false};
    std::thread publishing_thread_;

    std::queue<MsgType> outgoing_msg_queue_;
    std::mutex outgoing_mutex_;
    std::condition_variable outgoing_cv_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_TRANSMITTER_MESSAGE_QUEUE_HPP_
