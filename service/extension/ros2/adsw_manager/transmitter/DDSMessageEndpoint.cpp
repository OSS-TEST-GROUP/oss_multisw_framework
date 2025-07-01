#include "DDSMessageEndpoint.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

DDSMessageEndpoint::DDSMessageEndpoint() : participant_(nullptr), publisher_(nullptr), subscriber_(nullptr), topic_(nullptr),
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

    topic_ = participant_->create_topic(topic, GetTypeName(), TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        LOG_ERROR("Failed to create the topic. " << topic  );
        return false;
    }
    if (subscriber_ != nullptr) {
        data_reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, this);
        if (data_reader_ == nullptr) {
            LOG_ERROR("Failed to create a data reader. " << topic  );
            return false;
        }
    }
    if (publisher_ != nullptr) {
        data_writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, this);
        if (data_writer_ == nullptr) {
            LOG_ERROR("Failed to create a data writer. " << topic );
            return false;
        }
    }

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
    if (data_writer_) {
        data_writer_->write(value);
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

void DDSMessageEndpoint::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
    }
    else if (info.current_count_change == -1)
    {
    }
    else
    {
        LOG_DEBUG( info.current_count_change
                    << " is not a valid value for SubscriptionMatchedStatus current count change" );
    }
}

void DDSMessageEndpoint::on_data_available(DataReader* reader)
{
    OnDataAvailable();
}

} // namespace adsw_manager
} // namespace ros2
