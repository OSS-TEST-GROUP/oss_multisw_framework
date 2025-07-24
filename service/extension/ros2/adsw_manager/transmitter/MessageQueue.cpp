#include "MessageQueue.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

MessageQueue::MessageQueue() {}
MessageQueue::~MessageQueue() {}

bool MessageQueue::Start() {
    
    // Start publishing thread
    publishing_thread_ = std::thread(&MessageQueue::PublishingLoop, this);

    return true;
}

void MessageQueue::Stop() {
    stop_publisher_.store(true);
    
    if (publishing_thread_.joinable())
    {
        publishing_thread_.join();
    }
    outgoing_cv_.notify_one();

}

void MessageQueue::SendMessage(MsgType& message){

    {
        std::lock_guard<std::mutex> lock(outgoing_mutex_);
        outgoing_msg_queue_.push(message);
        LOG_DEBUG("SendMessage enqueue !! " << static_cast<int32_t>(message.first)  );
    }
    outgoing_cv_.notify_one();

}

void MessageQueue::PublishingLoop()
{
    while (!stop_publisher_.load())
    {
        std::unique_lock<std::mutex> lock(outgoing_mutex_);
        outgoing_cv_.wait(lock, [this] {
            return stop_publisher_.load() || !outgoing_msg_queue_.empty();
        });

        if (stop_publisher_.load())
            break;

        while (!outgoing_msg_queue_.empty())
        {
            const MsgType& msg = outgoing_msg_queue_.front();

            const char* topic = ADSWManager::Get()->GetTopic(msg.first);
            std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
            std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

            LOG_DEBUG("PublishingLoop !! " << static_cast<int32_t>(msg.first) << "::" << topic  );

            if (endpoint != nullptr){
                auto ret = endpoint->Write(msg.second);
                if (ret != eprosima::fastdds::dds::RETCODE_OK)
                {
                    LOG_DEBUG("Failed to write message: " << ret  );
                }
            }
            outgoing_msg_queue_.pop();
        }
    }
}



} // namespace adsw_manager
} // namespace ros2
