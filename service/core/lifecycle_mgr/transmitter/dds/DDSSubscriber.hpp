#ifndef LIFECYCLE_FASTDDS_SUBSCRIBER_HPP_
#define LIFECYCLE_FASTDDS_SUBSCRIBER_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

class DDSSubscriber : public DataReaderListener
{
public:

    DDSSubscriber();

    ~DDSSubscriber();

    bool Initialize();
    void Finalize();

    //! Subscription callback
    void on_data_available(
            DataReader* reader) override;

    //! Subscriber matched method
    void on_subscription_matched(
            DataReader* reader,
            const SubscriptionMatchedStatus& info) override;

private:

    //! Return the current state of execution
    bool is_stopped();

    //! Check if all instances have received all samples
    bool instances_received_all_samples();

    //! Check if all instances have been disposed
    bool instances_disposed();

    DomainParticipant* participant_;

    Subscriber* subscriber_;

    Topic* topic_;

    DataReader* reader_;

    TypeSupport type_;

    std::condition_variable cv_;

    std::mutex mutex_;

    uint32_t samples_;

    std::atomic<bool> stop_;

    std::atomic<bool> stop_receiving_samples_;

    std::map<InstanceHandle_t, uint32_t> samples_per_instance_;

    std::vector<InstanceHandle_t> disposed_instances_;
};

} // namespace lifecycle_manager


#endif // LIFECYCLE_FASTDDS_SUBSCRIBER_HPP_