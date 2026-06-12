#include "NotifyContainerStateEndPoint.hpp"

#include "lifecycle/NotifyContainerState.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

NotifyContainerStateEndPoint::NotifyContainerStateEndPoint(
    MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)) {
    // F008: same NotifyContainerState payload reused both as inbound
    // adsw→lifecycle subscription (NOTIFY_CONTAINER_STATE) and outbound
    // aggregated lifecycle→monitor publish (CONTAINER_STATE_REPORT).
    if (msgtype == MessageType::NOTIFY_CONTAINER_STATE ||
        msgtype == MessageType::CONTAINER_STATE_REPORT) {
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
