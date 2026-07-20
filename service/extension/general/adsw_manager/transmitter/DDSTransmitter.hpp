#ifndef GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_
#define GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_

#include <string>
#include <map>
#include <memory>

#include "DDSMessageEndpoint.hpp"
#include "MessageQueue.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace general {
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
    // In-class initializers ensure members are null after default construction
    // (works even when the stub DDSTransmitter::DDSTransmitter(){} body is empty).
    DomainParticipant* participant_ = nullptr;
    Publisher*         publisher_   = nullptr;
    Subscriber*        subscriber_  = nullptr;

    std::shared_ptr<MessageQueue> message_queue_;

    std::map<std::string, std::shared_ptr<DDSMessageEndpoint>> endpoints_;

};

} // namespace lifecycle_manager
} // namespace general

#endif // GENERAL_ADSW_MANAGER_TRANSMITTER_DDS_TRANSMITTER_HPP_
