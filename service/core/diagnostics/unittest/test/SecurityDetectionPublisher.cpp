#include "security_mgr/SecurityDetectionPubSubTypes.hpp"

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

class SecurityDetectionPublisher
{
public:
    security::DetectionReport data_;

private:
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

    SecurityDetectionPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new security::DetectionReportPubSubType())
    {
    }

    virtual ~SecurityDetectionPublisher()
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
        topic_ = participant_->create_topic("security/DetectionReport", "security::DetectionReport", TOPIC_QOS_DEFAULT);

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

    SecurityDetectionPublisher* mypub = new SecurityDetectionPublisher();
    if(!mypub->init()) {
        std::cout << "Failed to initialize ..." << std::endl;
        return -1;
    }

    while (true) {
        std::string client_id;
        std::cout << "Enter Security Client ID: ";
        std::cin >> client_id;
        
        snprintf(mypub->data_.client_id().data(), mypub->data_.client_id().size(), "%s", client_id.c_str());
        snprintf(mypub->data_.dds_topic().data(), mypub->data_.dds_topic().size(), "rt/DoorUnlockLockIndicator");
        snprintf(mypub->data_.rule().data(), mypub->data_.rule().size(), "custom rule anomaly detection");

        mypub->data_.timestamp_ms(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );

        // Show result
        std::cout << "[SecurityDetectionReport] client_id: " << mypub->data_.client_id().data()
          << ", dds_topic: " << mypub->data_.dds_topic().data()
          << ", rule: " << mypub->data_.rule().data()
          << ", timestamp_ms: " << mypub->data_.timestamp_ms() << std::endl;

        mypub->publish();
    }

    delete mypub;
    return 0;
}
