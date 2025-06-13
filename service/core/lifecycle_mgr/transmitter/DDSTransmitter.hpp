
#ifndef LIFECYCLE_DDS_TRANSMITTER_HPP_
#define LIFECYCLE_DDS_TRANSMITTER_HPP_

#include <string>
#include <map>
#include <memory>

#include "DDSMessageEndPoint.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace lifecycle_manager {

class DDSTransmitter {
  public:
    DDSTransmitter();
    virtual ~DDSTransmitter();

    bool Initialize();
    void Finalize();

    bool AddMessageEndpoint(std::shared_ptr<DDSMessageEndPoint> endpoint);
    std::shared_ptr<DDSMessageEndPoint> GetMessageEndPoint(std::string topic);

    DomainParticipant* GetDomain();
    Publisher* GetPublisher();
    Subscriber* GetSubscriber();

  private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;

    std::map<std::string, std::shared_ptr<DDSMessageEndPoint>> endpoints_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DDS_TRANSMITTER_HPP_
