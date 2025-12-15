
#ifndef LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>

#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "MessageEndPoint.hpp"
#include "include/MessageType.hpp"

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

class DDSMessageEndPoint : public MessageEndPoint, public DataWriterListener , public DataReaderListener{
  public:
    DDSMessageEndPoint(std::string topic);
    virtual ~DDSMessageEndPoint();

    bool Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber);
    void Finalize();


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

    bool Write(const void* const data);

    template<typename T>
    bool Read(T &value) {
        if (reader_ == nullptr) {
            return false;
        }
        SampleInfo info;
        if (reader_->take_next_sample(&value, &info) == eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool Read(std::vector<T>& value) {
        if (reader_ == nullptr) {
            return false;
        }
        SampleInfo info;
        bool ret_flag = false;
        while (true) {
            auto& val = value.emplace_back();
            if (reader_->take_next_sample(&val, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                if (info.valid_data) {
                    ret_flag = true;
                    continue;
                }
            } else {
                return ret_flag;
            }
        }
    }

    bool is_stopped() {return false;}

  private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;

    Topic* dds_topic_;

    DataWriter* writer_;
    DataReader* reader_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;

    int16_t matched_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_
