#include "StopContainerEndPoint.hpp"

#include <utility>

#include "lifecycle/StopContainerRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

StopContainerEndPoint::StopContainerEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::STOP_CONTAINER_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new StopContainerRequestPubSubType()));
    } else if (msgtype == MessageType::STOP_CONTAINER_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new StopContainerResponsePubSubType()));
    }
}


void StopContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<StopContainerResponse>(resp_)) {
        StopContainerResponse* msg =
            reinterpret_cast<StopContainerResponse*>(&resp_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void StopContainerEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
