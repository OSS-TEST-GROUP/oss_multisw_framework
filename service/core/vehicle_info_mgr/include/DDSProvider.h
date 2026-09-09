#ifndef DDSPROVIDER_H
#define DDSPROVIDER_H

#include "IProvider.h"
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

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
namespace autoware_adapi_v1_msgs { namespace msg { class VehicleKinematics; } }


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
    // Listener for DDS Discovery
    class DiscoveryListener;
    
    // Common Reader Listener using std::function callback
    class DDSDataReaderListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        using DataCallback = std::function<void(eprosima::fastdds::dds::DataReader*)>;
        DDSDataReaderListener(DataCallback callback) : callback_(callback) {}

        void on_subscription_matched(
                eprosima::fastdds::dds::DataReader* reader,
                const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;
    private:
        DataCallback callback_;
    };

    // Listener for VehicleDataWriter
    class VehicleDataWriterListener;

    // Use the fully qualified name for the forward declaration
    friend class autoware_sensing_msgs::msg::GnssInsOrientationStamped;

    eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
    eprosima::fastdds::dds::Publisher* publisher_{nullptr};
    eprosima::fastdds::dds::Subscriber* subscriber_{nullptr};
    
    // Resources for VehicleDataTopic
    eprosima::fastdds::dds::Topic* vehicle_data_topic_{nullptr};
    eprosima::fastdds::dds::Topic* set_vehicle_data_topic_{nullptr};
    eprosima::fastdds::dds::DataWriter* vehicle_data_writer_{nullptr};
    eprosima::fastdds::dds::DataReader* vehicle_data_reader_{nullptr};
    eprosima::fastdds::dds::TypeSupport vehicle_data_type_;
    std::unique_ptr<DDSDataReaderListener> vehicle_data_listener_;

    struct Subscription {
        std::string topic_name;
        eprosima::fastdds::dds::TypeSupport type_support;
        bool best_effort;
        std::function<void(eprosima::fastdds::dds::DataReader*)> callback;

        eprosima::fastdds::dds::Topic* topic = nullptr;
        eprosima::fastdds::dds::DataReader* reader = nullptr;
        std::unique_ptr<DDSDataReaderListener> listener;
    };

    std::vector<std::shared_ptr<Subscription>> subscriptions_;

    void registerSubscription(
        const std::string& topic_name,
        eprosima::fastdds::dds::TypeSupport type_support,
        bool best_effort,
        std::function<void(eprosima::fastdds::dds::DataReader*)> callback);

    void pushToDataChannel(const std::string& path, const std::any& value, long long timestamp = 0);

    // Topic data handlers
    void handleGnssOrientation(eprosima::fastdds::dds::DataReader* reader);
    void handleTrajectory(eprosima::fastdds::dds::DataReader* reader);
    void handleVehicleKinematics(eprosima::fastdds::dds::DataReader* reader);
    void handleVehicleStatus(eprosima::fastdds::dds::DataReader* reader);
    void handleRouteState(eprosima::fastdds::dds::DataReader* reader);
    void handleRoute(eprosima::fastdds::dds::DataReader* reader);
    void handleMrmState(eprosima::fastdds::dds::DataReader* reader);
    void handleEmergency(eprosima::fastdds::dds::DataReader* reader);
    void handleHazardStatus(eprosima::fastdds::dds::DataReader* reader);
    void handleOperationModeAvailability(eprosima::fastdds::dds::DataReader* reader);
    void handleOperationModeState(eprosima::fastdds::dds::DataReader* reader);
    void handleDynamicObjectArray(eprosima::fastdds::dds::DataReader* reader);
    void handleVelocityFactorArray(eprosima::fastdds::dds::DataReader* reader);
    void handleSteeringFactorArray(eprosima::fastdds::dds::DataReader* reader);
    void handleDetectedObjects(eprosima::fastdds::dds::DataReader* reader);
    void handlePath(eprosima::fastdds::dds::DataReader* reader);
    void handleTrafficLightGroupArray(eprosima::fastdds::dds::DataReader* reader);
    void handleDiagGraphStatus(eprosima::fastdds::dds::DataReader* reader);
    void handleDiagGraphStruct(eprosima::fastdds::dds::DataReader* reader);

    std::unique_ptr<VehicleDataWriterListener> vehicle_data_writer_listener_;

    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel_;
    std::shared_ptr<viss::VSSTree> vss_tree_;
    std::atomic<bool> active_;
    std::unique_ptr<DiscoveryListener> discovery_listener_;
};

#endif // DDSPROVIDER_H
