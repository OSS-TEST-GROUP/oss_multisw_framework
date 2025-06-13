
#ifndef LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <string>
#include <map>
#include <memory>

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

  private:
    std::map<MessageType, std::string> topic_lists_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_ENDPOINT_FACTORY_HPP_
