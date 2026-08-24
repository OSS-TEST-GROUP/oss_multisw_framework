#ifndef ROS2_ADSW_MANAGER_TRANSMITTER_MESSAGE_QUEUE_HPP_
#define ROS2_ADSW_MANAGER_TRANSMITTER_MESSAGE_QUEUE_HPP_

#include <atomic>
#include <string>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "endpoint/MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using MsgType = std::pair<MessageType, void*>;

class MessageQueue {
  public:
    MessageQueue();
    virtual ~MessageQueue() = default;
    void SendMessage(MsgType& message);

    bool Start() ;
    void Stop();

    void PublishingLoop();

  private:

    std::atomic<bool> stop_publisher_{false};
    std::thread publishing_thread_;

    std::queue<MsgType> outgoing_msg_queue_;
    std::mutex outgoing_mutex_;
    std::condition_variable outgoing_cv_;
};

} // namespace lifecycle_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_TRANSMITTER_MESSAGE_QUEUE_HPP_
