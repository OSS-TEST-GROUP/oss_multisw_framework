#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "logger/Log.hpp"
#include "manager/LifecycleFactory.hpp"

namespace lifecycle_manager {


StartServiceEndPoint::StartServiceEndPoint(MessageType msgtype, std::string topic, Callback cb) 
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)){
        
    if (msgtype == MessageType::START_SERVICE_REQUEST){
        SetTypeSupport(std::make_shared<TypeSupport>(new StartServiceRequestPubSubType()));
    }else if (msgtype == MessageType::START_SERVICE_RESPONSE){
        SetTypeSupport(std::make_shared<TypeSupport>(new StartServiceResponsePubSubType()));
    }
}

StartServiceEndPoint::~StartServiceEndPoint() {

}



void StartServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StartServiceResponse>(resp_) == true){

        // Drop samples coming from any participant other than the
        // adsw_manager we pinned in LifecycleFactory. Without this, phantom
        // publishers on the same DDS domain that replay stale FAILED
        // responses get matched by OnStart's in-flight counter and corrupt
        // the ctrl/StartServiceResponse verdict the monitor sees.
        if (!LifecycleFactory::GetInstance()
                ->IsKnownAdswPublisher(LastPublicationHandle())) {
            LOG_DEBUG("StartServiceEndPoint: dropping StartServiceResponse "
                      "from untrusted publisher (phantom guard)");
            return;
        }

        StartServiceResponse* message = reinterpret_cast<StartServiceResponse*>(&resp_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }

}

void StartServiceEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
