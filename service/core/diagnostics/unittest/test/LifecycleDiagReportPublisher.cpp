#include "LifecycleDiagReportPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;

class LifecycleDiagReportPublisher
{
private:

    lifecyclemgr::LifecycleDiagReport data_;

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_;

    DataWriter* writer_;

    TypeSupport type_;

    class PubListener : public DataWriterListener
    {
    public:

        PubListener()
            : matched_(0)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
                DataWriter*,
                const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                        << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;

    } listener_;

public:

    LifecycleDiagReportPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new lifecyclemgr::LifecycleDiagReportPubSubType())
    {
    }

    virtual ~LifecycleDiagReportPublisher()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //!Initialize the publisher
    bool init()
    {

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");

        // Coerce to use UDP protocol for testing
        // auto udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
        // udp_transport->sendBufferSize = 9216;
        // udp_transport->receiveBufferSize = 9216;
        // udp_transport->maxMessageSize = 9216;
        // udp_transport->non_blocking_send = true;

        // participantQos.transport().user_transports.push_back(udp_transport);
        // participantQos.transport().use_builtin_transports = false;

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        std::cout << "Created a participant. GUID=" << participant_->guid() << std::endl;

        // Register the Type
        type_.register_type(participant_);

        // Create the publications Topic
        topic_ = participant_->create_topic("mmf/lifecyclemgr/LifecycleDiagReport", "lifecyclemgr::LifecycleDiagReport", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        if (publisher_ == nullptr)
        {
            return false;
        }

        // Create the DataWriter
        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

        if (writer_ == nullptr)
        {
            return false;
        }
        return true;
    }

    //!Send a publication
    bool publish()
    {
        if (listener_.matched_ > 0)
        {
            std::cout << "Finish writing." << std::endl;
            writer_->write(&data_);
            return true;
        }
        return false;
    }

};

int main(
        int argc,
        char** argv)
{
    std::cout << "Starting publisher." << std::endl;
    uint32_t samples = 10;

    LifecycleDiagReportPublisher* mypub = new LifecycleDiagReportPublisher();
    if(!mypub->init()) {
        std::cout << "Failed to initialize ..." << std::endl;
        return -1;
    }

    while (true) {
        std::string started_input;
        std::cout << "Is started? (1 for true, 0 for false): ";
        std::cin >> started_input;
        mypub->data_.is_started(started_input == "1");

        uint32_t running_service_num;
        std::cout << "Enter number of running services: ";
        std::cin >> running_service_num;
        mypub->data_.running_service_num(running_service_num);

        uint32_t total_services_num;
        std::cout << "Enter total number of services: ";
        std::cin >> total_services_num;
        mypub->data_.total_services_num(total_services_num);

        // Show result
        std::cout << "[LifecycleDiagReport] is_started: " << (mypub->data_.is_started() ? "true" : "false")
          << ", running_service_num: " << mypub->data_.running_service_num()
          << ", total_services_num: " << mypub->data_.total_services_num() << std::endl;

        mypub->publish();
    }

    delete mypub;
    return 0;
}
