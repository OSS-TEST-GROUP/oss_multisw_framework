
#include <condition_variable>
#include <stdexcept>

#include "DDSMessageEndPoint.hpp"

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
    : MessageEndPoint(topic){

}

DDSMessageEndPoint::~DDSMessageEndPoint() {

}


bool DDSMessageEndPoint::Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber){
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

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

    // Create the reader
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    reader_ = subscriber_->create_datareader(dds_topic_, reader_qos, this, StatusMask::all());
    if (reader_ == nullptr)
    {
        throw std::runtime_error("DataReader initialization failed");
    }


    // Create the data writer
    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    writer_qos.history().depth = 5;
    publisher_->get_default_datawriter_qos(writer_qos);
    writer_ = publisher_->create_datawriter(dds_topic_, writer_qos, this, StatusMask::all());
    if (writer_ == nullptr)
    {
        throw std::runtime_error("DataWriter initialization failed");
    }
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

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
        std::cout << "Publisher matched." << std::endl;
        cv_.notify_one();
    }
    else if (info.current_count_change == -1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}


void DDSMessageEndPoint::on_subscription_matched(
        DataReader* /*reader*/,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void DDSMessageEndPoint::on_data_available(
        DataReader* reader)
{
    SampleInfo info;

    void* read_buffer = GetTopicBuffer();

    while ((RETCODE_OK == reader->take_next_sample(read_buffer, &info)))
    {
        if ((info.instance_state == ALIVE_INSTANCE_STATE) && info.valid_data)
        {
            // Print Hello world message data
            std::cout << "Message:  << "
                      << " RECEIVED" << std::endl;

            OnDataAvailable(read_buffer);
        }
    }
}


} // namespace lifecycle_manager
