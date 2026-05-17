#include "CtrlSetServiceModeEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlSetServiceModeEndPoint::CtrlSetServiceModeEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_SET_SERVICE_MODE_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlSetServiceModeRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_SET_SERVICE_MODE_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}

CtrlSetServiceModeEndPoint::~CtrlSetServiceModeEndPoint() {}

void CtrlSetServiceModeEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlSetServiceModeRequest>(req_) == true) {
        CtrlSetServiceModeRequest* msg =
            reinterpret_cast<CtrlSetServiceModeRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlSetServiceModeEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
