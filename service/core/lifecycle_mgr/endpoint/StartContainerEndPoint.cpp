#include "StartContainerEndPoint.hpp"

#include "lifecycle/StartContainerRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

StartContainerEndPoint::StartContainerEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::START_CONTAINER_REQUEST) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new StartContainerRequestPubSubType()));
    } else if (msgtype == MessageType::START_CONTAINER_RESPONSE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new StartContainerResponsePubSubType()));
    }
}

StartContainerEndPoint::~StartContainerEndPoint() {}

void StartContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<StartContainerResponse>(resp_) == true) {
        StartContainerResponse* msg =
            reinterpret_cast<StartContainerResponse*>(&resp_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void StartContainerEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
