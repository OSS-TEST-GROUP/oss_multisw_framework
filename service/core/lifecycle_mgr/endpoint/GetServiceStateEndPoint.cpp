#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "logger/Log.hpp"
#include "manager/LifecycleFactory.hpp"

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

        // Same phantom-publisher guard as the other response handlers. A
        // stray GetServiceStateResponse with NOT_RUNNING flips lifecycle_mgr's
        // service state, then PolicyAlways/OnFailure issues a START on its
        // next evaluation — the unending Stop/Start oscillation operators
        // see when recovery isn't MANUAL.
        if (!LifecycleFactory::GetInstance()
                ->IsKnownAdswPublisher(LastPublicationHandle())) {
            LOG_DEBUG("GetServiceStateEndPoint: dropping GetServiceStateResponse "
                      "from untrusted publisher (phantom guard)");
            return;
        }

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
