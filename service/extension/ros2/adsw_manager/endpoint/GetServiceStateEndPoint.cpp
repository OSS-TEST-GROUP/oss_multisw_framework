#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "ADSWManager.hpp"
#include "service/TypeConvert.hpp"
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

        GetServiceStateRequest* message = reinterpret_cast<GetServiceStateRequest*>(&req_);

        std::shared_ptr<ServiceRegistry> manager = ADSWManager::Get()->GetServiceRegistry();

        if (manager != nullptr){
            bool result = manager->ProcessGetServiceStateRequest(message);

            if (!result) {
                LOG_ERROR("Failed to process get service state request.");
            }
            LOG_DEBUG("Service state response sent for " << message->service_list().size() << " services.");
        }
        else {
            LOG_ERROR("Failed to retrieve service state.");
        }
    }
}

} // namespace adsw_manager
} // namespace ros2
