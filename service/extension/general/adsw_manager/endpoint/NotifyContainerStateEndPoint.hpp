#ifndef ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
#define ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_

#include <memory>
#include <string>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/NotifyContainerStatePubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

using namespace lifecyclemgr;

// F013: publish-only endpoint that emits NotifyContainerState to
// lifecycle_mgr whenever ContainerHealthMonitor sees a state transition.
// SUB side lives on the lifecycle_mgr; bridged out to the monitor by
// dds_bridge.
class NotifyContainerStateEndPoint : public DDSMessageEndpoint {
  public:
    NotifyContainerStateEndPoint(MessageType msgtype, std::string topic);
    ~NotifyContainerStateEndPoint() override= default;
    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    // Pure publisher — never invoked.
    void OnDataAvailable() override {}

  private:
    std::string                  topic_;
    std::shared_ptr<TypeSupport> type_support_;
};

} // namespace adsw_manager
} // namespace general

#endif // ADSW_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
