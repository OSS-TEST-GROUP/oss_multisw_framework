#include "DDSProvider.h"
#include "Utils.h" // For any_to_string

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/builtin/topic/ParticipantBuiltinTopicData.hpp>
#include <fastdds/dds/builtin/topic/PublicationBuiltinTopicData.hpp>
#include <fastdds/dds/builtin/topic/SubscriptionBuiltinTopicData.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.hpp>
#include <fastdds/rtps/reader/ReaderDiscoveryStatus.hpp>
#include <fastdds/rtps/writer/WriterDiscoveryStatus.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

// Include the generated DDS type support headers
#include "VehicleDataPubSubTypes.hpp"
#include "GnssInsOrientationStampedPubSubTypes.hpp" // For Autoware topic
#include "GnssInsOrientationStamped.hpp"
#include "TrajectoryPubSubTypes.hpp" // For Autoware trajectory
#include "Trajectory.hpp"
#include "VssTree.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <iostream>
#include <cmath> // For M_PI
#include "Logger.h"

using namespace eprosima::fastdds::dds;

// Listener for DDS Discovery
class DDSProvider::DiscoveryListener : public DomainParticipantListener
{
public:
    void on_participant_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::ParticipantDiscoveryStatus reason,
            const ParticipantBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch (reason)
        {
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DISCOVERED_PARTICIPANT:
                LOG_INFO << "[DDS Discovery] Discovered Participant: " << info.participant_name.to_string();
                break;
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::REMOVED_PARTICIPANT:
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DROPPED_PARTICIPANT:
                LOG_INFO << "[DDS Discovery] Removed or Dropped Participant: " << info.participant_name.to_string();
                break;
            default:
                break;
        }
    }

    void on_data_writer_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
            const PublicationBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch(reason)
        {
            case eprosima::fastdds::rtps::WriterDiscoveryStatus::DISCOVERED_WRITER:
                LOG_INFO << "[DDS Discovery] Discovered Publisher for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
            case eprosima::fastdds::rtps::WriterDiscoveryStatus::REMOVED_WRITER:
                LOG_INFO << "[DDS Discovery] Removed Publisher for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
        }
    }

    void on_data_reader_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::ReaderDiscoveryStatus reason,
            const SubscriptionBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch(reason)
        {
            case eprosima::fastdds::rtps::ReaderDiscoveryStatus::DISCOVERED_READER:
                LOG_INFO << "[DDS Discovery] Discovered Subscriber for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
            case eprosima::fastdds::rtps::ReaderDiscoveryStatus::REMOVED_READER:
                LOG_INFO << "[DDS Discovery] Removed Subscriber for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
        }
    }
};

// Listener for VehicleDataTopic
class DDSProvider::VehicleDataListener : public DataReaderListener
{
public:
    VehicleDataListener(ThreadSafeQueue<VehicleData>* channel) 
    : channel_(channel) {}

    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override
    {
        const auto participant = reader->get_subscriber()->get_participant();
        const std::string participant_name = participant->get_qos().name().to_string();

        if (info.current_count_change > 0)
        {
            LOG_INFO << "******************** [DDS Matched] DataReader matched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
        else
        {
            LOG_INFO << "******************** [DDS Unmatched] DataReader unmatched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
    }

    void on_data_available(DataReader* reader) override
    {
        SampleInfo info;
        VehicleDataTopic sample;
        if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                LOG_INFO << "DDS Data Received: " << sample.path() << " = " << sample.value();
                if (channel_)
                {
                    VehicleData data;
                    data.path = sample.path();
                    data.value = sample.value();
                    data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    std::cout << "DDSProvider: Pushing to channel -> Path: " << data.path << ", Value: " << std::any_cast<std::string>(data.value) << std::endl;
                    channel_->push(data);
                }
            }
        }
    }
private:
    ThreadSafeQueue<VehicleData>* channel_;
};

// Listener for GnssInsOrientationStamped
class DDSProvider::GnssInsOrientationListener : public DataReaderListener
{
public:
    GnssInsOrientationListener(ThreadSafeQueue<VehicleData>* channel)
    : channel_(channel) {}

    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override
    {
        const auto participant = reader->get_subscriber()->get_participant();
        const std::string participant_name = participant->get_qos().name().to_string();

        if (info.current_count_change == 1)
        {
            LOG_INFO << "******************** [DDS Matched] DataReader matched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
        else if (info.current_count_change == -1)
        {
            LOG_INFO << "******************** [DDS Unmatched] DataReader unmatched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
    }

    void on_data_available(DataReader* reader) override
    {
        SampleInfo info;
        autoware_sensing_msgs::msg::GnssInsOrientationStamped sample;
        if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                if (channel_)
                {
                    // Create a JSON object from the GnssInsOrientationStamped message
                    // based on the definition in vss_ads_types.json
                    rapidjson::Document gnss_doc;
                    gnss_doc.SetObject();
                    rapidjson::Document::AllocatorType& allocator = gnss_doc.GetAllocator();

                    // Create and populate the nested orientation object
                    rapidjson::Value orient_val(rapidjson::kObjectType);
                    orient_val.AddMember("x", rapidjson::Value(std::to_string(sample.orientation().orientation().x()).c_str(), allocator), allocator);
                    orient_val.AddMember("y", rapidjson::Value(std::to_string(sample.orientation().orientation().y()).c_str(), allocator), allocator);
                    orient_val.AddMember("z", rapidjson::Value(std::to_string(sample.orientation().orientation().z()).c_str(), allocator), allocator);
                    orient_val.AddMember("w", rapidjson::Value(std::to_string(sample.orientation().orientation().w()).c_str(), allocator), allocator);
                    gnss_doc.AddMember("orientation", orient_val, allocator);

                    // Add RMSE values
                    gnss_doc.AddMember("rmse_rotation_x", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_x()).c_str(), allocator), allocator);
                    gnss_doc.AddMember("rmse_rotation_y", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_y()).c_str(), allocator), allocator);
                    gnss_doc.AddMember("rmse_rotation_z", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_z()).c_str(), allocator), allocator);

                    VehicleData data;
                    data.path = "Vehicle.ADS.GnssInsOrientation";
                    data.value = std::make_shared<rapidjson::Document>(std::move(gnss_doc));
                    data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    std::any_cast<std::shared_ptr<rapidjson::Document>>(data.value)->Accept(writer);
                    LOG_INFO << "DDSProvider: Pushing to channel -> Path: " << data.path << ", Value: " << buffer.GetString();

                    channel_->push(data);
                }
            }
        }
    }
private:
    ThreadSafeQueue<VehicleData>* channel_;
};

// Listener for Trajectory
class DDSProvider::TrajectoryListener : public DataReaderListener
{
public:
    TrajectoryListener(
        ThreadSafeQueue<VehicleData>* channel)
        : channel_(channel) {}
    
    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override
    {
        const auto participant = reader->get_subscriber()->get_participant();
        const std::string participant_name = participant->get_qos().name().to_string();

        if (info.current_count_change == 1)
        {
            LOG_INFO << "******************** [DDS Matched] DataReader matched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
        else if (info.current_count_change == -1)
        {
            LOG_INFO << "******************** [DDS Unmatched] DataReader unmatched on participant [" << participant_name
                     << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
        }
    }

    void on_data_available(DataReader* reader) override
    {
        SampleInfo info;
        autoware_planning_msgs::msg::Trajectory sample;
        if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                if (!channel_) return;

                try {
                    rapidjson::Document trajectory_doc;
                    trajectory_doc.SetArray();
                    rapidjson::Document::AllocatorType& allocator = trajectory_doc.GetAllocator();

                    for (const auto& p : sample.points()) {
                        rapidjson::Document point_doc;
                        point_doc.SetObject();
                        
                        rapidjson::Value pose_val(rapidjson::kObjectType);
                        rapidjson::Value pos_val(rapidjson::kObjectType);
                        pos_val.AddMember("x", rapidjson::Value(std::to_string(p.pose().position().x()).c_str(), allocator), allocator);
                        pos_val.AddMember("y", rapidjson::Value(std::to_string(p.pose().position().y()).c_str(), allocator), allocator);
                        pos_val.AddMember("z", rapidjson::Value(std::to_string(p.pose().position().z()).c_str(), allocator), allocator);
                        pose_val.AddMember("position", pos_val, allocator);
                        
                        rapidjson::Value orient_val(rapidjson::kObjectType);
                        orient_val.AddMember("x", rapidjson::Value(std::to_string(p.pose().orientation().x()).c_str(), allocator), allocator);
                        orient_val.AddMember("y", rapidjson::Value(std::to_string(p.pose().orientation().y()).c_str(), allocator), allocator);
                        orient_val.AddMember("z", rapidjson::Value(std::to_string(p.pose().orientation().z()).c_str(), allocator), allocator);
                        orient_val.AddMember("w", rapidjson::Value(std::to_string(p.pose().orientation().w()).c_str(), allocator), allocator);
                        pose_val.AddMember("orientation", orient_val, allocator);

                        point_doc.AddMember("pose", pose_val, allocator);

                        rapidjson::Value time_val(rapidjson::kObjectType);
                        time_val.AddMember("sec", rapidjson::Value(std::to_string(p.time_from_start().sec()).c_str(), allocator), allocator);
                        time_val.AddMember("nanosec", rapidjson::Value(std::to_string(p.time_from_start().nanosec()).c_str(), allocator), allocator);
                        point_doc.AddMember("time_from_start", time_val, allocator);

                        point_doc.AddMember("longitudinal_velocity_mps", rapidjson::Value(std::to_string(p.longitudinal_velocity_mps()).c_str(), allocator), allocator);
                        point_doc.AddMember("lateral_velocity_mps", rapidjson::Value(std::to_string(p.lateral_velocity_mps()).c_str(), allocator), allocator);
                        point_doc.AddMember("acceleration_mps2", rapidjson::Value(std::to_string(p.acceleration_mps2()).c_str(), allocator), allocator);
                        point_doc.AddMember("heading_rate_rps", rapidjson::Value(std::to_string(p.heading_rate_rps()).c_str(), allocator), allocator);
                        point_doc.AddMember("front_wheel_angle_rad", rapidjson::Value(std::to_string(p.front_wheel_angle_rad()).c_str(), allocator), allocator);
                        point_doc.AddMember("rear_wheel_angle_rad", rapidjson::Value(std::to_string(p.rear_wheel_angle_rad()).c_str(), allocator), allocator);

                        trajectory_doc.PushBack(point_doc, allocator);
                    }

                    VehicleData data;
                    data.path = "Vehicle.ADS.PlanningTrajectory";
                    data.value = std::make_shared<rapidjson::Document>(std::move(trajectory_doc));
                    data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    std::any_cast<std::shared_ptr<rapidjson::Document>>(data.value)->Accept(writer);
                    LOG_INFO << "DDSProvider: Pushing to channel -> Path: " << data.path << ", Value: " << buffer.GetString();

                    channel_->push(data);

                } catch (const std::exception& e) {
                    LOG_ERROR << "Error processing Trajectory data: " << e.what();
                }
            }
        }
    }
private:
    ThreadSafeQueue<VehicleData>* channel_;
};

class DDSProvider::VehicleDataWriterListener : public DataWriterListener
{
public:
    void on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& info) override
    {
        const auto participant = writer->get_publisher()->get_participant();
        const std::string participant_name = participant->get_qos().name().to_string();

        if (info.current_count_change == 1)
        {
            LOG_INFO << "******************** [DDS Matched] DataWriter matched on participant [" << participant_name
                     << "] for topic: [" << writer->get_topic()->get_name() << "]";
        }
        else if (info.current_count_change == -1)
        {
            LOG_INFO << "******************** [DDS Unmatched] DataWriter unmatched on participant [" << participant_name
                     << "] for topic: [" << writer->get_topic()->get_name() << "]";
        }
    }
};


DDSProvider::DDSProvider()
    : participant_(nullptr)
    , publisher_(nullptr)
    , subscriber_(nullptr)
    , vehicle_data_topic_(nullptr)
    , set_vehicle_data_topic_(nullptr)
    , vehicle_data_writer_(nullptr)
    , vehicle_data_reader_(nullptr)
    , vehicle_data_type_(new VehicleDataTopicPubSubType())
    , gnss_orientation_topic_(nullptr)
    , gnss_orientation_reader_(nullptr)
    , gnss_orientation_type_(new autoware_sensing_msgs::msg::GnssInsOrientationStampedPubSubType())
    , trajectory_topic_(nullptr)
    , trajectory_reader_(nullptr)
    , trajectory_type_(new autoware_planning_msgs::msg::TrajectoryPubSubType())
    , active_(false)
    , discovery_listener_(std::make_unique<DiscoveryListener>())
{
    // Determine Domain ID from environment variable, default to 0
    int domain_id = 0;
    const char* domain_id_str = std::getenv("VISS_DDS_DOMAIN_ID");
    if (domain_id_str) {
        try {
            domain_id = std::stoi(domain_id_str);
        } catch (const std::invalid_argument& e) {
            LOG_WARN << "Invalid VISS_DDS_DOMAIN_ID: " << domain_id_str << ". Using default 0.";
            domain_id = 0;
        } catch (const std::out_of_range& e) {
            LOG_WARN << "VISS_DDS_DOMAIN_ID out of range: " << domain_id_str << ". Using default 0.";
            domain_id = 0;
        }
    }

    // Create Participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("VISS_Participant");
    participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos, discovery_listener_.get(), StatusMask::none());
    if (participant_ == nullptr) {
        LOG_ERROR << "Failed to create DDS Participant on domain " << domain_id;
        return;
    }

    // Register Types
    vehicle_data_type_.register_type(participant_);
    gnss_orientation_type_.register_type(participant_);
    trajectory_type_.register_type(participant_);

    // Create Publisher and Subscriber
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, StatusMask::none());
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, StatusMask::none());

    // Create Topics
    vehicle_data_topic_ = participant_->create_topic("VehicleData", vehicle_data_type_.get_type_name(), TOPIC_QOS_DEFAULT);
    set_vehicle_data_topic_ = participant_->create_topic("SetVehicleData", vehicle_data_type_.get_type_name(), TOPIC_QOS_DEFAULT);
    gnss_orientation_topic_ = participant_->create_topic("rt/autoware_orientation", gnss_orientation_type_.get_type_name(), TOPIC_QOS_DEFAULT);
    trajectory_topic_ = participant_->create_topic("rt/trajectory", trajectory_type_.get_type_name(), TOPIC_QOS_DEFAULT);
}

DDSProvider::~DDSProvider()
{
    stop();
    if (participant_ != nullptr)
    {
        participant_->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void DDSProvider::start()
{
    if (participant_ == nullptr) return;

    // Set Reliability QoS for DataReaders
    DataReaderQos reliable_qos = DATAREADER_QOS_DEFAULT;
    reliable_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Create DataReaders and Listeners
    vehicle_data_listener_ = std::make_unique<VehicleDataListener>(data_channel_.get());
    vehicle_data_reader_ = subscriber_->create_datareader(vehicle_data_topic_, reliable_qos, vehicle_data_listener_.get());

    gnss_orientation_listener_ = std::make_unique<GnssInsOrientationListener>(data_channel_.get());
    gnss_orientation_reader_ = subscriber_->create_datareader(gnss_orientation_topic_, reliable_qos, gnss_orientation_listener_.get());

    trajectory_listener_ = std::make_unique<TrajectoryListener>(data_channel_.get());
    trajectory_reader_ = subscriber_->create_datareader(trajectory_topic_, reliable_qos, trajectory_listener_.get());


    // Create DataWriter for VehicleDataTopic
    vehicle_data_writer_listener_ = std::make_unique<VehicleDataWriterListener>();
    vehicle_data_writer_ = publisher_->create_datawriter(set_vehicle_data_topic_, DATAWRITER_QOS_DEFAULT, vehicle_data_writer_listener_.get());

    active_ = true;
    LOG_INFO << "DDSProvider started.";
}

void DDSProvider::stop()
{
    if (!active_.exchange(false)) {
        return;
    }
    if (subscriber_ != nullptr)
    {
        if (vehicle_data_reader_ != nullptr)
        {
            subscriber_->delete_datareader(vehicle_data_reader_);
        }
        if (gnss_orientation_reader_ != nullptr)
        {
            subscriber_->delete_datareader(gnss_orientation_reader_);
        }
        if (trajectory_reader_ != nullptr)
        {
            subscriber_->delete_datareader(trajectory_reader_);
        }
    }
    if (publisher_ != nullptr && vehicle_data_writer_ != nullptr)
    {
        publisher_->delete_datawriter(vehicle_data_writer_);
    }
    LOG_INFO << "DDSProvider stopped.";
}

std::string DDSProvider::getName() const
{
    return "DDSProvider";
}

void DDSProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel)
{
    data_channel_ = channel;
}

void DDSProvider::setValue(const std::string& path, const std::any& value)
{
    // This provider is currently only for subscribing, so this is a no-op for now.
    // We only write to the VehicleData topic for now.
    if (vehicle_data_writer_ != nullptr)
    {
        VehicleDataTopic sample;
        sample.path(path);
        sample.value(any_to_string(value)); // Using helper to convert any to string
        vehicle_data_writer_->write(&sample);
        LOG_INFO << "DDSProvider: Published data for path " << path;
    }
}

void DDSProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree)
{
    vss_tree_ = vss_tree;
}
