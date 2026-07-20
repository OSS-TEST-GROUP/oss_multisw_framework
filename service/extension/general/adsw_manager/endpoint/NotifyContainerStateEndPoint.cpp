#include "NotifyContainerStateEndPoint.hpp"

namespace ros2 {
namespace adsw_manager {

NotifyContainerStateEndPoint::NotifyContainerStateEndPoint(MessageType msgtype, std::string topic)
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::NOTIFY_CONTAINER_STATE) {
        type_support_ = std::make_shared<TypeSupport>(new NotifyContainerStatePubSubType());
    }
}

NotifyContainerStateEndPoint::~NotifyContainerStateEndPoint() {}

std::string NotifyContainerStateEndPoint::GetTopic() { return topic_; }
std::string NotifyContainerStateEndPoint::GetTypeName() { return type_support_->get_type_name(); }
std::shared_ptr<TypeSupport> NotifyContainerStateEndPoint::GetTypeSupport() { return type_support_; }

} // namespace adsw_manager
} // namespace ros2
