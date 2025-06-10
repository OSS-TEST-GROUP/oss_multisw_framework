#include "DDSPublisher.hpp"

#include <algorithm>
#include <condition_variable>
#include <csignal>
#include <stdexcept>
#include <thread>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

namespace lifecycle_manager {

DDSPublisher::DDSPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new ShapeTypePubSubType())
    , matched_(0)
    , period_ms_(config.interval)
    , timeout_s_(config.timeout)
    , samples_(config.samples)
    , instances_(config.instances)
    , shape_config_(config.shape_config)
    , stop_(false)
{
}


DDSPublisher::~DDSPublisher(){

}

bool DDSPublisher::Initialize() {
    // Create the participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("Shape_pub_participant");
    // Set Shapes metadata
    pqos.properties().properties().emplace_back("fastdds.application.id", "SHAPES_DEMO", true);
    pqos.properties().properties().emplace_back("fastdds.application.metadata", "", true);
    participant_ = DomainParticipantFactory::get_instance()->create_participant(
        config.domain, pqos, nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        throw std::runtime_error("Participant initialization failed");
    }

    // Register the type
    type_.register_type(participant_);

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw std::runtime_error("Publisher initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(config.topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the data writer
    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    publisher_->get_default_datawriter_qos(writer_qos);
    writer_qos.reliability().kind = ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
    writer_qos.durability().kind = DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS;
    writer_qos.history().kind = HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS;
    uint32_t sample_limit = samples_ + 1; // include dispose sample
    if (samples_ == 0)
    {
        sample_limit = DATAWRITER_QOS_DEFAULT.resource_limits().max_samples_per_instance;
    }
    writer_qos.resource_limits().max_instances = instances_;
    writer_qos.history().depth = sample_limit;
    writer_qos.resource_limits().max_samples_per_instance = sample_limit;
    writer_qos.resource_limits().max_samples = sample_limit * instances_;

    // Check if ASYNCHRONOUS publish mode is required
    if (instances_ > 1)
    {
        writer_qos.publish_mode().kind = ASYNCHRONOUS_PUBLISH_MODE;
    }

    writer_ = publisher_->create_datawriter(topic_, writer_qos, this, StatusMask::all());
    if (writer_ == nullptr)
    {
        throw std::runtime_error("DataWriter initialization failed");
    }

    // Register the instances
    int n_dirs = 5; // There are 5 directions: LEFT, RIGHT, UP, DOWN, DIAGONAL
    for (int i = 0; i < instances_; i++)
    {
        InstanceHandle_t instance;
        ShapeType shape_;
        shape_location shape_loc;
        shape_loc.direction = CLIParser::ShapeDirection(i % n_dirs);    // different directions per instance
        shape_loc.x = static_cast<int>(shape_config_.width / 2);
        shape_loc.y = static_cast<int>(shape_config_.height / 2);

        // Create ShapeType and configuration associated to the instance
        shape_.shapesize(shape_config_.size);
        shape_.color(shape_config_.colors[i]);
        shape_.x(shape_loc.x);
        shape_.y(shape_loc.y);

        // Register instance
        std::cout << "Registering instance for " << shape_.color() << " shape" << std::endl;
        instance = writer_->register_instance(&shape_);
        instance_handles_.push_back(instance);

        // Store shape and configuration
        std::tuple<ShapeType, shape_location, uint32_t> shape_data(shape_, shape_loc, 0);
        shapes_[instance] = shape_data;
    }
}

void DDSPublisher::Finalize()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void DDSPublisher::on_publication_matched(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.current_count;
        std::cout << "Publisher matched." << std::endl;
        cv_.notify_one();
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.current_count;
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void DDSPublisher::publish()
{
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]()
            {
                // at least one has been discovered
                return ((matched_ > 0) || is_stopped());
            });

    if (!is_stopped())
    {
        // Iterate per instance
        for (int i = 0; i < instances_; i++)
        {
            InstanceHandle_t instance = instance_handles_[i];
            ShapeType shape_ = std::get<0>(shapes_[instance]);
            shape_location shape_loc = std::get<1>(shapes_[instance]);
            uint32_t samples_sent = std::get<2>(shapes_[instance]);

            // Move the shape
            move(shape_loc.x, shape_loc.y, shape_loc.direction);

            // Set shape values
            shape_.x(shape_loc.x);
            shape_.y(shape_loc.y);

            // Successfully performed write call
            if (RETCODE_OK == writer_->write(&shape_, instance))
            {
                samples_sent++;
                // Print sent shape data
                std::cout << shape_.color() << " Shape with size " << shape_.shapesize()
                          << " at X:" << shape_.x() << ", Y:" << shape_.y() << " SENT" << std::endl;

                // Update sample data
                std::tuple<ShapeType, shape_location, uint32_t> shape_data(shape_, shape_loc, samples_sent);
                shapes_[instance] = shape_data;

                // Check if last sample has been sent
                if (samples_ > 0 && samples_sent == samples_)
                {
                    std::cout << shape_.color() << " shape instance disposed" << std::endl;
                    writer_->dispose(&shape_, instance);
                }
            }
        }
    }
}

bool DDSPublisher::is_stopped()
{
    return stop_.load();
}

void DDSPublisher::stop()
{
    stop_.store(true);
    cv_.notify_one();
}


bool DDSPublisher::instances_sent_all_samples()
{
    bool ret = true;

    if (shapes_.size() > 0)
    {
        for (const auto& instance : shapes_)
        {
            if (std::get<2>(instance.second) < samples_)
            {
                ret = false;
                break;
            }
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

} // namespace lifecycle_manager