#include "ros2/ParticipantEntitiesInfoPubSubTypes.hpp"


#include <chrono>
#include <thread>

#include "logger/Log.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace rmw_dds_common::msg;

class ROSDiscoveryInfoSenderPublisher
{
private:

    ParticipantEntitiesInfo data_;

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
                LOG_DEBUG("Publisher matched.");
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                LOG_DEBUG("Publisher unmatched.");
            }
            else
            {
                LOG_DEBUG(info.current_count_change 
                        << " is not a valid value for PublicationMatchedStatus current count change.");
            }
        }

        std::atomic_int matched_;

    } listener_;

public:

    ROSDiscoveryInfoSenderPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new ParticipantEntitiesInfoPubSubType())
    {
    }

    virtual ~ROSDiscoveryInfoSenderPublisher()
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
        topic_ = participant_->create_topic("ros_discovery_info", "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_", TOPIC_QOS_DEFAULT);

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
            writer_->write(&data_);
            return true;
        }
        return false;
    }

    void setdata(int count)
    {

    }

};

int main(
        int argc,
        char** argv)
{
    std::cout << "Starting publisher." << std::endl;
    uint32_t samples = 20;

    ROSDiscoveryInfoSenderPublisher* mypub = new ROSDiscoveryInfoSenderPublisher();
    if(!mypub->init()) {
        std::cout << "Failed to initialize ..." << std::endl;
        return -1;
    }

    uint32_t count = 0;
    while (count < samples) {
        setdata(4);
        mypub->publish();
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    delete mypub;
    return 0;
}
