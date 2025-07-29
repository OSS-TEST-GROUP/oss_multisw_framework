#include "MessageQueue.hpp"
#include "DDSTransmitter.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

MessageQueue::MessageQueue(DDSTransmitter* transmitter) 
    : transmitter_(transmitter) {

}
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

            std::string topic = MessageTopic::GetTopicStr(msg.first);

            std::shared_ptr<DDSMessageEndPoint> endpoint = transmitter_->GetMessageEndPoint(topic);

            LOG_DEBUG("PublishingLoop !! " << static_cast<int32_t>(msg.first) << "::" << topic  );

            if (endpoint != nullptr){
                auto ret = endpoint->Write(msg.second);
                if (ret != true)
                {
                    LOG_DEBUG("Failed to write message: " << topic  );
                }
            }
            outgoing_msg_queue_.pop();
        }
    }
}



} // namespace lifecycle_manager
