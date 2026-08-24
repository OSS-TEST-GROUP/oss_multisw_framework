#include "StopServiceEndPoint.hpp"

#include <utility>

#include "lifecycle/StopServiceRequest.hpp"
#include "logger/Log.hpp"
#include "manager/LifecycleFactory.hpp"

namespace lifecycle_manager {

StopServiceEndPoint::StopServiceEndPoint(MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {

    if (msgtype == MessageType::STOP_SERVICE_REQUEST){
        SetTypeSupport(std::make_shared<TypeSupport>(new StopServiceRequestPubSubType()));
    }else if (msgtype == MessageType::STOP_SERVICE_RESPONSE){
        SetTypeSupport(std::make_shared<TypeSupport>(new StopServiceResponsePubSubType()));
    }
}

StopServiceEndPoint::~StopServiceEndPoint() {

}



void StopServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StopServiceResponse>(resp_)) {

        // Same phantom guard as StartServiceEndPoint — see commentary there.
        if (!LifecycleFactory::GetInstance()
                ->IsKnownAdswPublisher(LastPublicationHandle())) {
            LOG_DEBUG("StopServiceEndPoint: dropping StopServiceResponse "
                      "from untrusted publisher (phantom guard)");
            return;
        }

        StopServiceResponse* message = reinterpret_cast<StopServiceResponse*>(&resp_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }
}


void StopServiceEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
