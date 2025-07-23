#include "UpdateServiceListEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

UpdateServiceListEndPoint::UpdateServiceListEndPoint(std::string topic) 
    : DDSMessageEndpoint() {
        
    topic_ = topic;
    type_support_ = std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType());

}

UpdateServiceListEndPoint::~UpdateServiceListEndPoint() {

}

std::string UpdateServiceListEndPoint::GetTopic() {
    return topic_;
}

std::string UpdateServiceListEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> UpdateServiceListEndPoint::GetTypeSupport() {
    return type_support_;
}

void UpdateServiceListEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<UpdateServiceListResponse>(resp_) == true){

        UpdateServiceListResponse* message = reinterpret_cast<UpdateServiceListResponse*>(&resp_);

    }

}


} // namespace adsw_manager
} // namespace ros2
