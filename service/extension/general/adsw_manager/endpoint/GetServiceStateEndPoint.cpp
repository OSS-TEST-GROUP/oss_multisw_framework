#include "GetServiceStateEndPoint.hpp"

#include <utility>

#include "lifecycle/GetServiceStateRequest.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"


namespace general {
namespace adsw_manager {

GetServiceStateEndPoint::GetServiceStateEndPoint(MessageType msgtype, std::string topic) : topic_(std::move(topic)) {

    if (msgtype == MessageType::GET_SERVICE_STATE_REQUEST){
        type_support_ = std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType());
    }else if (msgtype == MessageType::GET_SERVICE_STATE_RESPONSE){
        type_support_ = std::make_shared<TypeSupport>(new GetServiceStateResponsePubSubType());
    }
}

GetServiceStateEndPoint::~GetServiceStateEndPoint() {

}


std::string GetServiceStateEndPoint::GetTopic() {
    return topic_;
}

std::string GetServiceStateEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> GetServiceStateEndPoint::GetTypeSupport() {
    return type_support_;
}

void GetServiceStateEndPoint::OnDataAvailable(){
    
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<GetServiceStateRequest>(req_)) {

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
} // namespace general
