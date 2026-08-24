#ifndef LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_START_SERVICE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class StartServiceEndPoint : public DDSMessageEndpoint {
  public:
    StartServiceEndPoint(MessageType msgtype, std::string topic);
    ~StartServiceEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    StartServiceRequest req_;
    StartServiceResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
