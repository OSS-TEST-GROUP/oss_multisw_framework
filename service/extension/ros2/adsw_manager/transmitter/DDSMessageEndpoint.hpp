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

    bool Initialize(DomainParticipant* participant, Subscriber* subscriber, Publisher* publisher);
    void Finalize();

    bool Write(void* value);

    template<typename T>
    bool Read(T &value) {
        SampleInfo info;
        if (data_reader_->take_next_sample(&value, &info) == eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool Read(std::vector<T>& value) {
        SampleInfo info;
        bool ret_flag = false;
        while (true) {
            auto& val = value.emplace_back();
            if (data_reader_->take_next_sample(&val, &info) == eprosima::fastdds::dds::RETCODE_OK)
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

    uint64_t GetPendingReadData();
    uint64_t GetPendingWriteData();

    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override;
    void on_data_available(DataReader* reader) override;
    
  private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;

    Topic* topic_;

    DataReader* data_reader_;
    DataWriter* data_writer_;

};

} // namespace lifecycle_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_TRANSMITTER_DDS_MESSAGE_ENDPOINT_HPP_
