#include "DDSMessageEndpoint.hpp"
#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {

DDSMessageEndpoint::DDSMessageEndpoint() : participant_(nullptr), publisher_(nullptr), subscriber_(nullptr), dds_topic_(nullptr),
        data_reader_(nullptr), data_writer_(nullptr) {}
DDSMessageEndpoint::~DDSMessageEndpoint() {}

bool DDSMessageEndpoint::Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber) {
    participant_ = participant;
    subscriber_ = subscriber;
    publisher_ = publisher;

    std::string topic = GetTopic();
    auto type_support = GetTypeSupport();
    if (type_support == nullptr) {
        LOG_ERROR("No typesupport. topic=" << topic );
        return false;
    }
    if (type_support->register_type(participant) != RETCODE_OK) {
        LOG_ERROR("Failed to register typesupport. topic=" << topic  );
        return false;
    }

    dds_topic_ = participant_->create_topic(topic, GetTypeName(), TOPIC_QOS_DEFAULT);
    if (dds_topic_ == nullptr) {
        LOG_ERROR("Failed to create the topic. " << topic  );
        return false;
    }

    if (subscriber_ != nullptr)
    {
        // Create the reader
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        subscriber_->get_default_datareader_qos(reader_qos);
        // F011: explicit data_sharing OFF — sibling-container writers' POSIX-shm
        // segments are unreachable across container mount namespaces.
        reader_qos.data_sharing().off();
        data_reader_ = subscriber_->create_datareader(dds_topic_, reader_qos, this, StatusMask::all());
        if (data_reader_ == nullptr)
        {
            LOG_ERROR("Failed to create a data reader. " << topic  );
            return false;
        }
    }

    if (publisher_ != nullptr)
    {
        // Create the data writer
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.history().depth = 5;
        publisher_->get_default_datawriter_qos(writer_qos);
        // F011: see reader counterpart — explicit data_sharing OFF.
        writer_qos.data_sharing().off();
        data_writer_ = publisher_->create_datawriter(dds_topic_, writer_qos, this, StatusMask::all());
        if (data_writer_ == nullptr)
        {
            LOG_ERROR("Failed to create a data writer. " << topic );
            return false;
        }
    }

    LOG_DEBUG("DDSMessage Intialized " << topic << ", " << GetTypeName());
    return true;
}

void DDSMessageEndpoint::Finalize() {
    if (data_reader_) {
        subscriber_->delete_datareader(data_reader_);
        data_reader_ = nullptr;
    }
    if (data_writer_) {
        publisher_->delete_datawriter(data_writer_);
        data_writer_ = nullptr;
    }
}

bool DDSMessageEndpoint::Write(void* value) {
    LOG_DEBUG("write message. " << GetTopic() );

    if (data_writer_) {
        auto ret = data_writer_->write(value);
        if (ret != eprosima::fastdds::dds::RETCODE_OK)
        {
            LOG_DEBUG("Failed to write message: " << GetTopic() << ", return = " << ret  );
            return false;
        }
        return true;
    }
    return false;
}

uint64_t DDSMessageEndpoint::GetPendingReadData() {
    if (data_reader_) {
        return data_reader_->get_unread_count();
    }
    return 0;
}

uint64_t DDSMessageEndpoint::GetPendingWriteData() {
    return 0;
}


void DDSMessageEndpoint::on_publication_matched(
        DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        LOG_DEBUG("Publisher matched. [" << GetTopic() << "]" << matched_);
    }
    else if (info.current_count_change == -1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        LOG_DEBUG("Publisher unmatched. [" << GetTopic() << "]" << matched_);
    }
    else
    {
        LOG_DEBUG(info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change [" << GetTopic() << "]");
    }
}


void DDSMessageEndpoint::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        LOG_DEBUG( "Subscriber matched. [" << GetTopic() << "]" << static_cast<int16_t>(info.current_count));
    }
    else if (info.current_count_change == -1)
    {
        LOG_DEBUG( "Subscriber unmatched. [" << GetTopic() << "]" << static_cast<int16_t>(info.current_count));
    }
    else
    {
        LOG_DEBUG( info.current_count_change
                    << " is not a valid value for SubscriptionMatchedStatus current count change [" << GetTopic() << "]");
    }
}

void DDSMessageEndpoint::on_data_available(DataReader* reader)
{
    LOG_DEBUG( "on_data_available [" << GetTopic() << "]");
    
    OnDataAvailable();
}


} // namespace adsw_manager
} // namespace general
