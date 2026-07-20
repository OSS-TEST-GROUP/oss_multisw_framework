#ifndef ADSW_STOP_CONTAINER_ENDPOINT_HPP_
#define ADSW_STOP_CONTAINER_ENDPOINT_HPP_

#include <memory>
#include <string>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StopContainerRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

// F013: mirror of StartContainerEndPoint for the stop direction.
class StopContainerEndPoint : public DDSMessageEndpoint {
  public:
    StopContainerEndPoint(MessageType msgtype, std::string topic);
    virtual ~StopContainerEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string                  topic_;
    std::shared_ptr<TypeSupport> type_support_;

    StopContainerRequest  req_;
    StopContainerResponse resp_;
};

} // namespace adsw_manager
} // namespace ros2

#endif // ADSW_STOP_CONTAINER_ENDPOINT_HPP_
