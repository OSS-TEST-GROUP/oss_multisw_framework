#ifndef ROS2_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_HPP_
#define ROS2_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <map>
#include <memory>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>


using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

class DDSMessageEndpoint : public DataWriterListener, public DataReaderListener {
  public:
    DDSMessageEndpoint();
    virtual ~DDSMessageEndpoint();

    virtual std::string GetTopic() = 0;
    virtual std::string GetTypeName() = 0;
    virtual std::shared_ptr<TypeSupport> GetTypeSupport() = 0;

    virtual void OnDataAvailable() = 0;

    bool Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber);
    void Finalize();

    bool Write(void* value);

    template<typename T>
    bool Read(T &value) {
        SampleInfo info;

        if (data_reader_ == nullptr) {
            return false;
        }
        if (data_reader_->take_next_sample(&value, &info) == eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data) {
                // Cache the publication_handle so the endpoint's
                // OnDataAvailable can correlate this sample back to a
                // specific publisher. The phantom-publisher guard in
                // StartService/StopService endpoints uses this to drop any
                // samples coming from a participant other than the trusted
                // lifecycle_mgr (whose prefix we pin via the
                // UpdateServiceListResponse trust anchor).
                last_publication_handle_ = info.publication_handle;
                return true;
            }
        }
        return false;
    }

    const eprosima::fastdds::dds::InstanceHandle_t& LastPublicationHandle() const {
        return last_publication_handle_;
    }

    uint64_t GetPendingReadData();
    uint64_t GetPendingWriteData();


    //! Publisher matched method
    void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override;

    //! Subscription callback
    void on_data_available(
            DataReader* reader) override;

    //! Subscriber matched method
    void on_subscription_matched(
            DataReader* reader,
            const SubscriptionMatchedStatus& info) override;


    // This is for unittesting purpose..!! do not use it
    // testability: allow injection of mock DataReader

  private:
    // In-class initializers ensure these are null even when the stub
    // DDSMessageEndpoint::DDSMessageEndpoint(){} in MockDDSMessageEndpoint.hpp
    // is linked instead of the real constructor (which has the same initializers
    // in its member-initializer-list).  Without these, the stub constructor
    // leaves the pointers uninitialised, causing Read<T> to dereference garbage.
    DomainParticipant* participant_ = nullptr;
    Publisher*         publisher_   = nullptr;
    Subscriber*        subscriber_  = nullptr;

    Topic*      dds_topic_   = nullptr;

    DataReader* data_reader_ = nullptr;
    DataWriter* data_writer_ = nullptr;

    int16_t matched_ = 0;

    // Updated by Read() with the publication_handle of the last sample we
    // consumed. Subclasses look at LastPublicationHandle() during their
    // OnDataAvailable handler.
    eprosima::fastdds::dds::InstanceHandle_t last_publication_handle_;
};

} // namespace lifecycle_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_HPP_
