#include "NotifyServiceStateEndPoint.hpp"

#include "lifecycle/NotifyServiceState.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {
    
NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(std::string topic) 
    : topic_(topic) {

    type_support_ = std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType());
}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}

std::string NotifyServiceStateEndPoint::GetTopic() {
    return topic_;
}

std::string NotifyServiceStateEndPoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> NotifyServiceStateEndPoint::GetTypeSupport() {
    return type_support_;
}

void NotifyServiceStateEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );
}


} // namespace adsw_manager
} // namespace ros2
