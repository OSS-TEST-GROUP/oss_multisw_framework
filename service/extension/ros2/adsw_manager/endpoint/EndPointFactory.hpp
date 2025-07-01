#ifndef ROS2_ADSW_MANAGER_ENDPOINT_ENDPOINT_FACTORY_HPP_
#define ROS2_ADSW_MANAGER_ENDPOINT_ENDPOINT_FACTORY_HPP_

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include "transmitter/DDSTransmitter.hpp"

namespace ros2 {
namespace adsw_manager {

enum class PubSubType {
    PUBSUB_NONE,
    PUBLISH_ONLY,
    SUBSCRIBE_ONLY,

    PUBSUB_BOTH,
};    

enum class MessageType {
    ROS2_DISCOVREY_INFO,
};

class EndPointFactory {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();
    
    bool Initialize();
    void Finalize();
    
    int32_t CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter);

  private:
    std::map<MessageType, std::string> topic_lists_;

};

} // namespace adsw_manager
} // namespace ros2

#endif
