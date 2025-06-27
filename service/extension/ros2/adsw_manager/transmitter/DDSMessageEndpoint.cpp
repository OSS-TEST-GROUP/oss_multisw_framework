#include "DDSMessageEndpoint.hpp"

namespace ros2 {
namespace adsw_manager {

DDSMessageEndpoint::DDSMessageEndpoint() : participant_(nullptr), publisher_(nullptr), subscriber_(nullptr), topic_(nullptr),
        data_reader_(nullptr), data_writer_(nullptr) {}
DDSMessageEndpoint::~DDSMessageEndpoint() {}

bool DDSMessageEndpoint::Initialize(DomainParticipant* participant, Subscriber* subscriber, Publisher* publisher) {
    participant_ = participant;
    subscriber_ = subscriber;
    publisher_ = publisher;

    std::string topic = GetTopic();
    auto type_support = GetTypeSupport();
    if (type_support == nullptr) {
        std::cerr << "No typesupport. topic=" << topic << std::endl;
        return false;
    }
    if (type_support->register_type(participant) != RETCODE_OK) {
        std::cerr << "Failed to register typesupport. topic=" << topic << std::endl;
        return false;
    }

    topic_ = participant_->create_topic(topic, GetTypeName(), TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        std::cerr << "Failed to create the topic. " << topic << std::endl;
        return false;
    }

    data_reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, this);
    if (data_reader_ == nullptr) {
        std::cerr << "Failed to create a data reader. " << topic << std::endl;
        return false;
    }
    data_writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, this);
    if (data_writer_ == nullptr) {
        std::cerr << "Failed to create a data writer. " << topic << std::endl;
        return false;
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
        std::cout << info.current_count_change
                    << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void DDSMessageEndpoint::on_data_available(DataReader* reader)
{
    OnDataAvailable();
}

} // namespace lifecycle_manager
} // namespace ros2
