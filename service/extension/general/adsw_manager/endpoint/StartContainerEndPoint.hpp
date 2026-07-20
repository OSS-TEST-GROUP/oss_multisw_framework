#ifndef ADSW_START_CONTAINER_ENDPOINT_HPP_
#define ADSW_START_CONTAINER_ENDPOINT_HPP_

#include <memory>
#include <string>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StartContainerRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

using namespace lifecyclemgr;

// F013: adsw_manager side of Start/StopContainer. Subscribes to
// StartContainerRequest from lifecycle_mgr, runs ContainerController, and
// publishes StartContainerResponse with the verdict.  Two instances are
// created by the factory (one PUB, one SUB) so a single class can drive
// both directions consistently with StartServiceEndPoint.
class StartContainerEndPoint : public DDSMessageEndpoint {
  public:
    StartContainerEndPoint(MessageType msgtype, std::string topic);
    virtual ~StartContainerEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string                  topic_;
    std::shared_ptr<TypeSupport> type_support_;

    StartContainerRequest  req_;
    StartContainerResponse resp_;
};

} // namespace adsw_manager
} // namespace general

#endif // ADSW_START_CONTAINER_ENDPOINT_HPP_
