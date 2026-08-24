#include "NotifyContainerStateEndPoint.hpp"

#include <utility>

namespace general {
namespace adsw_manager {

NotifyContainerStateEndPoint::NotifyContainerStateEndPoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::NOTIFY_CONTAINER_STATE) {
        type_support_ = std::make_shared<TypeSupport>(new NotifyContainerStatePubSubType());
    }
}


std::string NotifyContainerStateEndPoint::GetTopic() { return topic_; }
std::string NotifyContainerStateEndPoint::GetTypeName() { return type_support_->get_type_name(); }
std::shared_ptr<TypeSupport> NotifyContainerStateEndPoint::GetTypeSupport() { return type_support_; }

} // namespace adsw_manager
} // namespace general
