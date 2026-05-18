#include "SetRecoveryPolicyEndPoint.hpp"

#include "lifecycle/SetRecoveryPolicyRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

SetRecoveryPolicyEndPoint::SetRecoveryPolicyEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::SET_RECOVERY_POLICY_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new SetRecoveryPolicyRequestPubSubType()));
    } else if (msgtype == MessageType::SET_RECOVERY_POLICY_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new SetRecoveryPolicyResponsePubSubType()));
    }
}

SetRecoveryPolicyEndPoint::~SetRecoveryPolicyEndPoint() {}

void SetRecoveryPolicyEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<SetRecoveryPolicyResponse>(resp_) == true) {
        SetRecoveryPolicyResponse* msg =
            reinterpret_cast<SetRecoveryPolicyResponse*>(&resp_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void SetRecoveryPolicyEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
