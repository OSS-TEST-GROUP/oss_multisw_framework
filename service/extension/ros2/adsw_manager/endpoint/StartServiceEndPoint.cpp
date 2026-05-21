#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "ADSWManager.hpp"

#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {
    
StartServiceEndPoint::StartServiceEndPoint(MessageType msgtype, std::string topic) 
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::START_SERVICE_REQUEST){
        type_support_ = std::make_shared<TypeSupport>(new StartServiceRequestPubSubType());
    }else if (msgtype == MessageType::START_SERVICE_RESPONSE){
        type_support_ = std::make_shared<TypeSupport>(new StartServiceResponsePubSubType());
    }
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

        // Phantom-publisher guard. Only the participant identified by the
        // first UpdateServiceListResponse (i.e. the real lifecycle_mgr) may
        // command a service start. Any other publisher writing directly to
        // the raw mmf/lifecyclemgr/StartServiceRequest topic — zombie DDS
        // participants, leaked test fixtures, hand-rolled probes — is dropped
        // here. This is what keeps a MANUAL service from resurrecting on its
        // own after operator-driven Stop.
        if (!ADSWManager::Get()->IsKnownLifecyclePublisher(LastPublicationHandle())) {
            LOG_DEBUG("StartServiceEndPoint: dropping StartServiceRequest "
                      "from untrusted publisher (phantom guard)");
            return;
        }

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