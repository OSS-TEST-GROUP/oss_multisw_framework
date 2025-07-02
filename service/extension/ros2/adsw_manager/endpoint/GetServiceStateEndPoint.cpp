#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "logger/Log.hpp"


namespace ros2 {
namespace adsw_manager {

GetServiceStateEndPoint::GetServiceStateEndPoint(std::string topic) 
    : topic_(topic) {

    type_support_ = std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType());
}

GetServiceStateEndPoint::~GetServiceStateEndPoint() {

}


std::string GetServiceStateEndPoint::GetTopic() {
    return topic_;
}

std::string GetServiceStateEndPoint::GetTypeName() {
    return "rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_";
}

std::shared_ptr<TypeSupport> GetServiceStateEndPoint::GetTypeSupport() {
    return type_support_;
}

void GetServiceStateEndPoint::OnDataAvailable(){
    
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<GetServiceStateRequest>(req_) == true){

        GetServiceStateRequest* response = reinterpret_cast<GetServiceStateRequest*>(&req_);

     }

}

} // namespace adsw_manager
} // namespace ros2
