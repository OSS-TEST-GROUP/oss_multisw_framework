#include "StopServiceEndPoint.hpp"

#include "lifecycle/StopServiceRequest.hpp"
#include "ADSWManager.hpp"
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

        std::shared_ptr<ROSServiceController> manager = ADSWManager::Get()->GetServiceController();
        if (manager != nullptr){
            bool result = manager->ProcessServiceStopRequest(message);
            if (!result) {
                LOG_ERROR("Failed to process service stop request.");
            }
        }
        else {
            LOG_ERROR("Failed to retrieve service controller.");
        }
    }

}


} // namespace adsw_manager
} // namespace ros2
