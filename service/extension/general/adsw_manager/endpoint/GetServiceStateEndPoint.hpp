#ifndef LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class GetServiceStateEndPoint : public DDSMessageEndpoint {
  public:
    GetServiceStateEndPoint(MessageType msgtype, std::string topic);
    virtual ~GetServiceStateEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    GetServiceStateRequest req_;
    GetServiceStateResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif