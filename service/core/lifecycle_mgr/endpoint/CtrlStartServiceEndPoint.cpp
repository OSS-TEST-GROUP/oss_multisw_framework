#include "CtrlStartServiceEndPoint.hpp"

#include <utility>

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlStartServiceEndPoint::CtrlStartServiceEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_START_SERVICE_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlStartServiceRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_START_SERVICE_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}


void CtrlStartServiceEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlStartServiceRequest>(req_)) {
        CtrlStartServiceRequest* msg =
            reinterpret_cast<CtrlStartServiceRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlStartServiceEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
