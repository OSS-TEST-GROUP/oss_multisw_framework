#include "StopServiceEndPoint.hpp"

#include "lifecycle/StopServiceRequest.hpp"
#include "ADSWManager.hpp"
#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {
    
StopServiceEndPoint::StopServiceEndPoint(MessageType msgtype, std::string topic) 
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::STOP_SERVICE_REQUEST){
        type_support_ = std::make_shared<TypeSupport>(new StopServiceRequestPubSubType());
    }else if (msgtype == MessageType::STOP_SERVICE_RESPONSE){
        type_support_ = std::make_shared<TypeSupport>(new StopServiceResponsePubSubType());
    }
}

StopServiceEndPoint::~StopServiceEndPoint() {

}

std::string StopServiceEndPoint::GetTopic() {
    return topic_;
}

std::string StopServiceEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> StopServiceEndPoint::GetTypeSupport() {
    return type_support_;
}

void StopServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StopServiceRequest>(req_) == true){

        // Same phantom guard as StartServiceEndPoint — see commentary there.
        if (!ADSWManager::Get()->IsKnownLifecyclePublisher(LastPublicationHandle())) {
            LOG_DEBUG("StopServiceEndPoint: dropping StopServiceRequest "
                      "from untrusted publisher (phantom guard)");
            return;
        }

        StopServiceRequest* message = reinterpret_cast<StopServiceRequest*>(&req_);

        std::shared_ptr<ContainerServiceController> manager = ADSWManager::Get()->GetServiceController();
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
} // namespace general
