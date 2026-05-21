#include "NotifyContainerStateEndPoint.hpp"

#include "lifecycle/NotifyContainerState.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

NotifyContainerStateEndPoint::NotifyContainerStateEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    if (msgtype == MessageType::NOTIFY_CONTAINER_STATE) {
        SetTypeSupport(std::make_shared<TypeSupport>(
            new NotifyContainerStatePubSubType()));
    }
}

NotifyContainerStateEndPoint::~NotifyContainerStateEndPoint() {}

void NotifyContainerStateEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<NotifyContainerState>(data_) == true) {
        NotifyContainerState* msg =
            reinterpret_cast<NotifyContainerState*>(&data_);
        if (on_data_cb_) {
            on_data_cb_(msg);
        }
    }
}

void NotifyContainerStateEndPoint::OnDataPublished(bool ret) {
    LOG_DEBUG("Message Published for " << GetTopic() << ", "
              << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
