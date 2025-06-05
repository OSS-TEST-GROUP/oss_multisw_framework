#ifndef LIFECYCLE_FASTDDS_PUBLISHER_HPP_
#define LIFECYCLE_FASTDDS_PUBLISHER_HPP_

#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

class DDSPublisher : public DataWriterListener
{
public:

    DDSPublisher();

    ~DDSPublisher();
    
    bool Initialize();
    void Finalize();    

    //! Publisher matched method
    void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override;

private:

    //! Return the current state of execution
    bool is_stopped();

    //! Publish a sample
    void publish();

    //! Check if all instances have sent the expected number of samples
    bool instances_sent_all_samples();

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_;

    DataWriter* writer_;

    TypeSupport type_;

    std::condition_variable cv_;

    int32_t matched_;

    std::mutex mutex_;

    uint32_t period_ms_;

    uint32_t timeout_s_;

    uint16_t samples_;

    uint16_t instances_;

    std::vector<InstanceHandle_t> instance_handles_;

    std::atomic<bool> stop_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_FASTDDS_PUBLISHER_HPP_