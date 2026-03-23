#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "logger/Log.hpp"

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
