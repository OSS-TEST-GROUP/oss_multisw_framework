#ifndef DDSPROVIDER_H
#define DDSPROVIDER_H

#include "IProvider.h"
#include <atomic>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>

// Forward declarations for Fast DDS types
namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
class Publisher;
class Subscriber;
class Topic;
class DataWriter;
class DataReader;
} // namespace dds
} // namespace fastdds
} // namespace eprosima

// Forward declarations for generated types
class VehicleDataTopic;
namespace autoware_sensing_msgs { namespace msg { class GnssInsOrientationStamped; } }


class DDSProvider : public IProvider {
public:
    DDSProvider();
    ~DDSProvider() override;

    void start() override;
    void stop() override;
    std::string getName() const override;
    void setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel) override;
    void setValue(const std::string& path, const std::any& value) override;
    void setVssTree(std::shared_ptr<viss::VSSTree> vss_tree) override;

private:
    // Listener for VehicleDataTopic
    class VehicleDataListener;
    // Listener for GnssInsOrientationStamped
    class GnssInsOrientationListener;

    // Use the fully qualified name for the forward declaration
    friend class autoware_sensing_msgs::msg::GnssInsOrientationStamped;

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    
    // Resources for VehicleDataTopic
    eprosima::fastdds::dds::Topic* vehicle_data_topic_;
    eprosima::fastdds::dds::DataWriter* vehicle_data_writer_;
    eprosima::fastdds::dds::DataReader* vehicle_data_reader_;
    eprosima::fastdds::dds::TypeSupport vehicle_data_type_;
    std::unique_ptr<VehicleDataListener> vehicle_data_listener_;

    // Resources for GnssInsOrientationStamped
    eprosima::fastdds::dds::Topic* gnss_orientation_topic_;
    eprosima::fastdds::dds::DataReader* gnss_orientation_reader_;
    eprosima::fastdds::dds::TypeSupport gnss_orientation_type_;
    std::unique_ptr<GnssInsOrientationListener> gnss_orientation_listener_;

    // Resources for Trajectory
    class TrajectoryListener;
    eprosima::fastdds::dds::Topic* trajectory_topic_;
    eprosima::fastdds::dds::DataReader* trajectory_reader_;
    eprosima::fastdds::dds::TypeSupport trajectory_type_;
    std::unique_ptr<TrajectoryListener> trajectory_listener_;

    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel_;
    std::shared_ptr<viss::VSSTree> vss_tree_;
    std::atomic<bool> active_;
};

#endif // DDSPROVIDER_H
