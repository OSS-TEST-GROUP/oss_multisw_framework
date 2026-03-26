#include "NotifyServiceStateEndPoint.hpp"

#include "lifecycle/NotifyServiceState.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(MessageType msgtype, std::string topic, Callback cb) 
    : DDSMessageEndPoint(topic), on_data_cb_(std::move(cb)){

    if (msgtype == MessageType::NOTIFY_SERVICE_STATE){
        SetTypeSupport(std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType()));
    }
}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}



void NotifyServiceStateEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<NotifyServiceState>(data_) == true){

        NotifyServiceState* message = reinterpret_cast<NotifyServiceState*>(&data_);

        if (on_data_cb_) {
            on_data_cb_(message);
        }
    }

}

void NotifyServiceStateEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
