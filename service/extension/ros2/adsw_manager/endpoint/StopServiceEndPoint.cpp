#include "StopServiceEndPoint.hpp"

#include "lifecycle/StopServiceRequest.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {
    
StopServiceEndPoint::StopServiceEndPoint(std::string topic) 
    : topic_(topic){

    type_support_ = std::make_shared<TypeSupport>(new StopServiceRequestPubSubType());

}

StopServiceEndPoint::~StopServiceEndPoint() {

}

std::string StopServiceEndPoint::GetTopic() {
    return topic_;
}

std::string StopServiceEndPoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> StopServiceEndPoint::GetTypeSupport() {
    return type_support_;
}

void StopServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StopServiceRequest>(req_) == true){

        StopServiceRequest* message = reinterpret_cast<StopServiceRequest*>(&req_);

    }

}


} // namespace adsw_manager
} // namespace ros2
