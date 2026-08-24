#ifndef LIFECYCLE_CTRL_STOP_CONTAINER_ENDPOINT_HPP_
#define LIFECYCLE_CTRL_STOP_CONTAINER_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: ctrl/* gate endpoint for monitor → lifecycle_mgr StopContainer.
class CtrlStopContainerEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(CtrlStopContainerRequest*)>;

    CtrlStopContainerEndPoint(MessageType msgtype, std::string topic,
                               Callback cb = nullptr);
    ~CtrlStopContainerEndPoint() override= default;
    void OnDataAvailable() override;
    void OnDataPublished(bool ret) override;

  private:
    CtrlStopContainerRequest req_;
    CtrlResponse             resp_;
    Callback                 on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CTRL_STOP_CONTAINER_ENDPOINT_HPP_
