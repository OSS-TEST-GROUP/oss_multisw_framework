#ifndef LIFECYCLE_SET_RECOVERY_POLICY_ENDPOINT_HPP_
#define LIFECYCLE_SET_RECOVERY_POLICY_ENDPOINT_HPP_

#include <functional>
#include <memory>
#include <string>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/SetRecoveryPolicyRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

// F014 follow-up: paired endpoint for SetRecoveryPolicyRequest /
// SetRecoveryPolicyResponse — mirrors the StartContainerEndPoint pattern.
// The factory creates one PUBLISH_ONLY instance (lifecycle_mgr → adsw)
// and one SUBSCRIBE_ONLY instance (adsw → lifecycle_mgr).
class SetRecoveryPolicyEndPoint : public DDSMessageEndPoint {
  public:
    using Callback = std::function<void(SetRecoveryPolicyResponse*)>;

    SetRecoveryPolicyEndPoint(MessageType msgtype, std::string topic,
                               Callback cb = nullptr);
    virtual ~SetRecoveryPolicyEndPoint();

    virtual void OnDataAvailable() override;
    virtual void OnDataPublished(bool ret) override;

  private:
    SetRecoveryPolicyRequest  req_;
    SetRecoveryPolicyResponse resp_;
    Callback                  on_data_cb_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SET_RECOVERY_POLICY_ENDPOINT_HPP_
