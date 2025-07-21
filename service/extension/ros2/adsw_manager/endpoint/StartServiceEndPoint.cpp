#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "ADSWManager.hpp"

#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {
    
StartServiceEndPoint::StartServiceEndPoint(std::string topic) 
    : DDSMessageEndpoint() {
        
    topic_ = topic;
    type_support_ = std::make_shared<TypeSupport>(new StartServiceRequestPubSubType());

}

StartServiceEndPoint::~StartServiceEndPoint() {

}

std::string StartServiceEndPoint::GetTopic() {
    return topic_;
}

std::string StartServiceEndPoint::GetTypeName() {
    return type_support_->get_type_name();
}

std::shared_ptr<TypeSupport> StartServiceEndPoint::GetTypeSupport() {
    return type_support_;
}

void StartServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StartServiceRequest>(req_) == true){

        StartServiceRequest* message = reinterpret_cast<StartServiceRequest*>(&req_);

        std::shared_ptr<ROSServiceController> manager = ADSWManager::Get()->GetServiceController();
        if (manager != nullptr){
            bool result = manager->ProcessServiceStartRequest(message);
            if (!result) {
                LOG_ERROR("Failed to process service start request.");
            }
        }
        else {
            LOG_ERROR("Failed to retrieve service controller.");
        }
    }

}

} // namespace adsw_manager
}