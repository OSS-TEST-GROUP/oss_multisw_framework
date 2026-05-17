#include "CtrlStopContainerEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlStopContainerEndPoint::CtrlStopContainerEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_STOP_CONTAINER_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlStopContainerRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_STOP_CONTAINER_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}

CtrlStopContainerEndPoint::~CtrlStopContainerEndPoint() {}

void CtrlStopContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlStopContainerRequest>(req_) == true) {
        CtrlStopContainerRequest* msg =
            reinterpret_cast<CtrlStopContainerRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlStopContainerEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
