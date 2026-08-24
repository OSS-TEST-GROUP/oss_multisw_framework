#ifndef GENERAL_ADSW_MANAGER_ENDPOINT_ENDPOINT_FACTORY_HPP_
#define GENERAL_ADSW_MANAGER_ENDPOINT_ENDPOINT_FACTORY_HPP_

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include "transmitter/DDSTransmitter.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

enum class PubSubType {
    PUBSUB_NONE,
    PUBLISH_ONLY,
    SUBSCRIBE_ONLY,

    PUBSUB_BOTH,
};    

class EndPointFactory {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();
    
    bool Initialize();
    void Finalize();

    const char* GetTopic(MessageType mtype);

    int32_t CreateEndPoints(const std::shared_ptr<DDSTransmitter>& transmitter);

  private:
    std::map<MessageType, std::string> topic_lists_;

};

} // namespace adsw_manager
} // namespace general

#endif
