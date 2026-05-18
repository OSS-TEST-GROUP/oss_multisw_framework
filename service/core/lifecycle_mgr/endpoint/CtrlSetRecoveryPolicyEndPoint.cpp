#include "CtrlSetRecoveryPolicyEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlSetRecoveryPolicyEndPoint::CtrlSetRecoveryPolicyEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_SET_RECOVERY_POLICY_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlSetRecoveryPolicyRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}

CtrlSetRecoveryPolicyEndPoint::~CtrlSetRecoveryPolicyEndPoint() {}

void CtrlSetRecoveryPolicyEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlSetRecoveryPolicyRequest>(req_) == true) {
        CtrlSetRecoveryPolicyRequest* msg =
            reinterpret_cast<CtrlSetRecoveryPolicyRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlSetRecoveryPolicyEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
