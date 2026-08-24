#ifndef LIFECYCLE_CTRL_SET_RECOVERY_POLICY_ENDPOINT_HPP_
#define LIFECYCLE_CTRL_SET_RECOVERY_POLICY_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F014 follow-up: ctrl/* gate endpoint for monitor → lifecycle_mgr
// SetRecoveryPolicy request.  Mirrors CtrlStartContainerEndPoint shape.
class CtrlSetRecoveryPolicyEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(CtrlSetRecoveryPolicyRequest*)>;

    CtrlSetRecoveryPolicyEndPoint(MessageType msgtype, std::string topic,
                                    Callback cb = nullptr);
    ~CtrlSetRecoveryPolicyEndPoint() override= default;
    void OnDataAvailable() override;
    void OnDataPublished(bool ret) override;

  private:
    CtrlSetRecoveryPolicyRequest req_;
    CtrlResponse                 resp_;
    Callback                     on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CTRL_SET_RECOVERY_POLICY_ENDPOINT_HPP_
