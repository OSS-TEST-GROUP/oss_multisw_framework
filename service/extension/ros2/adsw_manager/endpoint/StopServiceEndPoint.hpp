#ifndef LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StopServiceRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class StopServiceEndPoint : public DDSMessageEndpoint {
  public:
    StopServiceEndPoint(MessageType msgtype, std::string topic);
    ~StopServiceEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    StopServiceRequest req_;
    StopServiceResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
