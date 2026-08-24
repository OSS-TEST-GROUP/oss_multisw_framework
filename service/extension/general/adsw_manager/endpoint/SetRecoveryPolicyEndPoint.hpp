#ifndef ADSW_SET_RECOVERY_POLICY_ENDPOINT_HPP_
#define ADSW_SET_RECOVERY_POLICY_ENDPOINT_HPP_

#include <memory>
#include <string>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/SetRecoveryPolicyRequestPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

using namespace lifecyclemgr;

// F014 follow-up: adsw side of SetRecoveryPolicy.  SUB the request,
// mutate ServiceRegistry, persist via ServiceRegistry::Save(), PUB the
// response.  Same paired pattern as Start/StopContainerEndPoint.
class SetRecoveryPolicyEndPoint : public DDSMessageEndpoint {
  public:
    SetRecoveryPolicyEndPoint(MessageType msgtype, std::string topic);
    ~SetRecoveryPolicyEndPoint() override= default;
    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string                  topic_;
    std::shared_ptr<TypeSupport> type_support_;

    SetRecoveryPolicyRequest  req_;
    SetRecoveryPolicyResponse resp_;
};

} // namespace adsw_manager
} // namespace general

#endif // ADSW_SET_RECOVERY_POLICY_ENDPOINT_HPP_
