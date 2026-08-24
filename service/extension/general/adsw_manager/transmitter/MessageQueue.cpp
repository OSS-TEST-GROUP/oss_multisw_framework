#include "MessageQueue.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace general {
namespace adsw_manager {

MessageQueue::MessageQueue() {}

bool MessageQueue::Start() {
    if (publishing_thread_.joinable()) {
        // Already running, do not start again
        return false;
    }
    stop_publisher_.store(false);
    publishing_thread_ = std::thread(&MessageQueue::PublishingLoop, this);
    return true;
}

void MessageQueue::Stop() {
    stop_publisher_.store(true);
    outgoing_cv_.notify_one();  // Wake up the thread if it's waiting

    if (publishing_thread_.joinable())
    {
        publishing_thread_.join();
    }
}

void MessageQueue::SendMessage(MsgType& message){

    {
        std::lock_guard<std::mutex> lock(outgoing_mutex_);
        outgoing_msg_queue_.push(message);
        LOG_DEBUG("SendMessage enqueue !! " << static_cast<int32_t>(message.first) << " , " << static_cast<int32_t>(outgoing_msg_queue_.size()) );
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

        if (stop_publisher_.load()) {
            break;
        }

        while (!outgoing_msg_queue_.empty())
        {
            const MsgType& msg = outgoing_msg_queue_.front();

            const char* topic = ADSWManager::Get()->GetTopic(msg.first);
            std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
            std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

            LOG_DEBUG("PublishingLoop !! " << static_cast<int32_t>(msg.first) << "::" << topic  );

            if (endpoint != nullptr){
                // DDSMessageEndpoint::Write returns bool (true on RETCODE_OK).
                // Comparing the bool to RETCODE_OK (=0) inverted the verdict so
                // every successful write was logged as "Failed".
                bool ok = endpoint->Write(msg.second);
                if (!ok)
                {
                    LOG_DEBUG("Failed to write message: " << topic  );
                }
            }
            outgoing_msg_queue_.pop();
        }
    }
}



} // namespace adsw_manager
} // namespace general
