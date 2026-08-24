#ifndef LIFECYCLE_CTRL_START_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_CTRL_START_SERVICE_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F013: ctrl/* gate endpoint for monitor → lifecycle_mgr StartService request.
// On the lifecycle_mgr side this class subscribes to CtrlStartServiceRequest
// (CTRL_START_SERVICE_REQUEST) and publishes CtrlResponse on the response
// topic (CTRL_START_SERVICE_RESPONSE). Two instances are created by the
// factory, one per direction.
class CtrlStartServiceEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(CtrlStartServiceRequest*)>;

    CtrlStartServiceEndPoint(MessageType msgtype, std::string topic,
                              Callback cb = nullptr);
    ~CtrlStartServiceEndPoint() override= default;
    void OnDataAvailable() override;
    void OnDataPublished(bool ret) override;

  private:
    CtrlStartServiceRequest req_;
    CtrlResponse            resp_;
    Callback                on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CTRL_START_SERVICE_ENDPOINT_HPP_
