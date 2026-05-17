#ifndef ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
#define ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_

#include <memory>
#include <string>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/NotifyContainerStatePubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

// F013: publish-only endpoint that emits NotifyContainerState to
// lifecycle_mgr whenever ContainerHealthMonitor sees a state transition.
// SUB side lives on the lifecycle_mgr; bridged out to the monitor by
// dds_bridge.
class NotifyContainerStateEndPoint : public DDSMessageEndpoint {
  public:
    NotifyContainerStateEndPoint(MessageType msgtype, std::string topic);
    virtual ~NotifyContainerStateEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    // Pure publisher — never invoked.
    virtual void OnDataAvailable() {}

  private:
    std::string                  topic_;
    std::shared_ptr<TypeSupport> type_support_;
};

} // namespace adsw_manager
} // namespace ros2

#endif // ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
