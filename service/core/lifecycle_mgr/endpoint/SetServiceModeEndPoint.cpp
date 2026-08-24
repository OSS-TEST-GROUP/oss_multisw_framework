#include "SetServiceModeEndPoint.hpp"

#include <utility>

#include "lifecycle/SetServiceModeRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

SetServiceModeEndPoint::SetServiceModeEndPoint(MessageType msgtype, std::string topic, Callback cb)
    : DDSMessageEndPoint(std::move(topic)), on_data_cb_(std::move(cb)) {

    if (msgtype == MessageType::SET_SERVICE_MODE_REQUST){
        SetTypeSupport(std::make_shared<TypeSupport>(new SetServiceModeRequestPubSubType()));
    }else if (msgtype == MessageType::SET_SERVICE_MODE_RESPONSE){
        SetTypeSupport(std::make_shared<TypeSupport>(new SetServiceModeResponsePubSubType()));
    }
}

SetServiceModeEndPoint::~SetServiceModeEndPoint() {

}



void SetServiceModeEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<SetServiceModeRequest>(data_)) {

        SetServiceModeRequest* message = reinterpret_cast<SetServiceModeRequest*>(&data_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }
}


void SetServiceModeEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}


} // namespace lifecycle_manager
