#ifndef LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <string>
#include <map>
#include <memory>
#include <functional>

#include "transmitter/DDSTransmitter.hpp"
#include "include/MessageType.hpp"

namespace lifecycle_manager {

class EndPointFactory  {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();

    int32_t Initialize();

    int32_t CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter);

    const char* GetTopic(MessageType mtype);

    // Allow injection of custom endpoint creators for testing
    using EndpointCreator = std::function<std::shared_ptr<DDSMessageEndPoint>(MessageType, const std::string&)>;
    void SetCustomCreator(MessageType type, EndpointCreator creator) {
        custom_creators_[type] = creator;
    }

  private:
    std::map<MessageType, std::string> topic_lists_;
    std::map<MessageType, EndpointCreator> custom_creators_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_ENDPOINT_FACTORY_HPP_
