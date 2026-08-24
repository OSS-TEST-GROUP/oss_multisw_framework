#include "NotifyServiceStateEndPoint.hpp"

#include <utility>

#include "lifecycle/NotifyServiceState.hpp"
#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {

NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::NOTIFY_SERVICE_STATE){
        type_support_ = std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType());
    }
}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}

std::string NotifyServiceStateEndPoint::GetTopic() {
    return topic_;
}

std::string NotifyServiceStateEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> NotifyServiceStateEndPoint::GetTypeSupport() {
    return type_support_;
}

void NotifyServiceStateEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );
}


} // namespace adsw_manager
} // namespace general
