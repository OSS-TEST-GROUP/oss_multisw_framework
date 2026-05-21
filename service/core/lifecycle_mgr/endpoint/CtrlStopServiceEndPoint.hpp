#ifndef LIFECYCLE_CTRL_STOP_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_CTRL_STOP_SERVICE_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: ctrl/* gate endpoint for monitor → lifecycle_mgr StopService request.
class CtrlStopServiceEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(CtrlStopServiceRequest*)>;

    CtrlStopServiceEndPoint(MessageType msgtype, std::string topic,
                             Callback cb = nullptr);
    virtual ~CtrlStopServiceEndPoint();

    virtual void OnDataAvailable() override;
    virtual void OnDataPublished(bool ret) override;

  private:
    CtrlStopServiceRequest req_;
    CtrlResponse           resp_;
    Callback               on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CTRL_STOP_SERVICE_ENDPOINT_HPP_
