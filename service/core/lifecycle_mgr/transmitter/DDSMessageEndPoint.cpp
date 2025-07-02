
#include <condition_variable>
#include <stdexcept>

#include "DDSMessageEndPoint.hpp"
#include "logger/Log.hpp"

#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

DDSMessageEndPoint::DDSMessageEndPoint(std::string topic) 
    : participant_(nullptr),
    publisher_(nullptr),
    subscriber_(nullptr), 
    MessageEndPoint(topic){

}

DDSMessageEndPoint::~DDSMessageEndPoint() {

}


bool DDSMessageEndPoint::Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber){

    participant_ = participant;
    publisher_ = publisher;
    subscriber_ = subscriber;
        
    // Register the Type
    GetTypeSupport()->register_type(participant_);

    // Create the publications Topic
    dds_topic_ = participant_->create_topic(GetTopic(), GetTypeName(), TOPIC_QOS_DEFAULT);

    if (dds_topic_ == nullptr)
    {
        return false;
    }

    if (publisher_ != nullptr)
    {
            // Create the data writer
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.history().depth = 5;
        publisher_->get_default_datawriter_qos(writer_qos);
        writer_ = publisher_->create_datawriter(dds_topic_, writer_qos, this, StatusMask::all());
        if (writer_ == nullptr)
        {
            throw std::runtime_error("DataWriter initialization failed");
        }

    }

    if (subscriber_ != nullptr)
    {
        // Create the reader
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        subscriber_->get_default_datareader_qos(reader_qos);
        reader_ = subscriber_->create_datareader(dds_topic_, reader_qos, this, StatusMask::all());
        if (reader_ == nullptr)
        {
            throw std::runtime_error("DataReader initialization failed");
        }
    }

    return true;
}


void DDSMessageEndPoint::Finalize(){

}


bool DDSMessageEndPoint::Write(const void* const data)
{
    bool ret = false;
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]()
            {
                // at least one has been discovered
                return (is_stopped());
            });

    if (!is_stopped())
    {
        ret = (RETCODE_OK == writer_->write(data));

        OnDataPublished(ret);
    }
    return ret;
}

void DDSMessageEndPoint::on_publication_matched(
        DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        LOG_DEBUG("Publisher matched.");
        cv_.notify_one();
    }
    else if (info.current_count_change == -1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        LOG_DEBUG("Publisher unmatched.");
    }
    else
    {
        LOG_DEBUG(info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change");
    }
}


void DDSMessageEndPoint::on_subscription_matched(
        DataReader* /*reader*/,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        LOG_DEBUG("Subscriber matched.");
    }
    else if (info.current_count_change == -1)
    {
        LOG_DEBUG("Subscriber unmatched.");
    }
    else
    {
        LOG_DEBUG(info.current_count_change << " is not a valid value for SubscriptionMatchedStatus current count change");
    }
}

void DDSMessageEndPoint::on_data_available(
        DataReader* reader)
{
    OnDataAvailable();
}


} // namespace lifecycle_manager
