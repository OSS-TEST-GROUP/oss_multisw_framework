#include "UpdateServiceListEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

UpdateServiceListEndPoint::UpdateServiceListEndPoint(MessageType msgtype, std::string topic, Callback cb) 
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)){

    if (msgtype == MessageType::UPDATE_SERVICE_LIST_REQUEST){
        SetTypeSupport(std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType()));
    }else if (msgtype == MessageType::UPDATE_SERVICE_LIST_RESPONSE){
        SetTypeSupport(std::make_shared<TypeSupport>(new UpdateServiceListResponsePubSubType()));
    }
}

UpdateServiceListEndPoint::~UpdateServiceListEndPoint() {

}


void UpdateServiceListEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<UpdateServiceListRequest>(data_) == true){

        UpdateServiceListRequest* message = reinterpret_cast<UpdateServiceListRequest*>(&data_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }
}

void UpdateServiceListEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}



} // namespace lifecycle_manager
