#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {
    
StartServiceEndPoint::StartServiceEndPoint(std::string topic) 
    : topic_(topic){
        
    type_support_ = std::make_shared<TypeSupport>(new StartServiceRequestPubSubType());

}

StartServiceEndPoint::~StartServiceEndPoint() {

}

std::string StartServiceEndPoint::GetTopic() {
    return topic_;
}

std::string StartServiceEndPoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> StartServiceEndPoint::GetTypeSupport() {
    return type_support_;
}

void StartServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StartServiceRequest>(req_) == true){

        StartServiceRequest* message = reinterpret_cast<StartServiceRequest*>(&req_);

    }

}


} // namespace adsw_manager
} // namespace ros2
