#include "CtrlStopServiceEndPoint.hpp"

#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

CtrlStopServiceEndPoint::CtrlStopServiceEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::CTRL_STOP_SERVICE_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlStopServiceRequestPubSubType()));
    } else if (msgtype == MessageType::CTRL_STOP_SERVICE_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new CtrlResponsePubSubType()));
    }
}

CtrlStopServiceEndPoint::~CtrlStopServiceEndPoint() {}

void CtrlStopServiceEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<CtrlStopServiceRequest>(req_) == true) {
        CtrlStopServiceRequest* msg =
            reinterpret_cast<CtrlStopServiceRequest*>(&req_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void CtrlStopServiceEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
