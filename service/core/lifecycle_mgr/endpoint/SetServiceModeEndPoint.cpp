#include "SetServiceModeEndPoint.hpp"

#include "lifecycle/SetServiceModeRequest.hpp"

#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {
    
SetServiceModeEndPoint::SetServiceModeEndPoint(MessageType msgtype, std::string topic) 
    : DDSMessageEndPoint(topic){
    
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

    if (Read<SetServiceModeRequest>(data_) == true){

        SetServiceModeRequest* message = reinterpret_cast<SetServiceModeRequest*>(&data_);

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetStateMgr();

        if (manager != nullptr){
            manager->SetServiceMode(message);            
        }
    }

}


void SetServiceModeEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}


} // namespace lifecycle_manager
