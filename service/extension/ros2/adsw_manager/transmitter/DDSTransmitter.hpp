#ifndef ROS2_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_
#define ROS2_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_

#include <string>
#include <map>
#include <memory>

#include "DDSMessageEndpoint.hpp"
#include "MessageQueue.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

class DDSTransmitter {
  public:
    DDSTransmitter();
    virtual ~DDSTransmitter();

    bool Initialize();
    void Finalize();

    bool AddMessageEndpoint(std::shared_ptr<DDSMessageEndpoint> endpoint);
    std::shared_ptr<DDSMessageEndpoint> GetMessageEndpoint(std::string topic);

    DomainParticipant* GetDomain();
    Publisher* GetPublisher();
    Subscriber* GetSubscriber();

    bool SendMessage(MessageType mtype, void* ptr);

  private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;

    std::shared_ptr<MessageQueue> message_queue_;

    std::map<std::string, std::shared_ptr<DDSMessageEndpoint>> endpoints_;

};

} // namespace lifecycle_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_
