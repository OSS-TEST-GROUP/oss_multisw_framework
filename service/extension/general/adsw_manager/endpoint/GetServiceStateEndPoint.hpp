#ifndef LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_GET_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/GetServiceStateRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

using namespace lifecyclemgr;

class GetServiceStateEndPoint : public DDSMessageEndpoint {
  public:
    GetServiceStateEndPoint(MessageType msgtype, std::string topic);
    ~GetServiceStateEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    GetServiceStateRequest req_;
    GetServiceStateResponse resp_;

};

} // namespace adsw_manager
} // namespace general

#endif