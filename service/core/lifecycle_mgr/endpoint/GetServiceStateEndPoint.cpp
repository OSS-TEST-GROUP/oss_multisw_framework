#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


GetServiceStateEndPoint::GetServiceStateEndPoint(MessageType msgtype, std::string topic, Callback cb) 
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)){

    if (msgtype == MessageType::GET_SERVICE_STATE_REQUEST){
        SetTypeSupport(std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType()));
    }else if (msgtype == MessageType::GET_SERVICE_STATE_RESPONSE){
        SetTypeSupport(std::make_shared<TypeSupport>(new GetServiceStateResponsePubSubType()));
    }
}

GetServiceStateEndPoint::~GetServiceStateEndPoint() {

}


void GetServiceStateEndPoint::OnDataAvailable(){
    
    if (Read<GetServiceStateResponse>(resp_) == true){

        GetServiceStateResponse* response = reinterpret_cast<GetServiceStateResponse*>(&resp_);

        if (on_data_cb_) {
            on_data_cb_(response);
        }
    }
}

void GetServiceStateEndPoint::OnDataPublished(bool ret){
    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
