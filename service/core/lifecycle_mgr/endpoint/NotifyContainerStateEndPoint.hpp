#ifndef LIFECYCLE_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
#define LIFECYCLE_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/NotifyContainerStatePubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: subscriber for adsw_manager's NotifyContainerState publishes.
// lifecycle_mgr updates ServiceRegistry::UpdateContainerState() and forwards
// to lfc_monitor via the same topic (handled by dds_bridge on the ROS side,
// so no separate forward endpoint needed here).
class NotifyContainerStateEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(NotifyContainerState*)>;

    NotifyContainerStateEndPoint(MessageType msgtype, std::string topic,
                                  Callback cb = nullptr);
    virtual ~NotifyContainerStateEndPoint();

    virtual void OnDataAvailable() override;
    virtual void OnDataPublished(bool ret) override;

  private:
    NotifyContainerState data_;
    Callback             on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_NOTIFY_CONTAINER_STATE_ENDPOINT_HPP_
