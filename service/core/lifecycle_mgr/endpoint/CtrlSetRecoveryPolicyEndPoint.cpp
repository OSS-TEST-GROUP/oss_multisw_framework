#include "CtrlSetRecoveryPolicyEndPoint.hpp"

#include <utility>

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlSetRecoveryPolicyEndPoint::CtrlSetRecoveryPolicyEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_SET_RECOVERY_POLICY_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlSetRecoveryPolicyRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}


void CtrlSetRecoveryPolicyEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlSetRecoveryPolicyRequest>(req_)) {
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
