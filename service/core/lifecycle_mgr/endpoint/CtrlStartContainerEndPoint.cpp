#include "CtrlStartContainerEndPoint.hpp"

#include <utility>

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlStartContainerEndPoint::CtrlStartContainerEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_START_CONTAINER_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlStartContainerRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_START_CONTAINER_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}


void CtrlStartContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlStartContainerRequest>(req_)) {
        CtrlStartContainerRequest* msg =
            reinterpret_cast<CtrlStartContainerRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlStartContainerEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
