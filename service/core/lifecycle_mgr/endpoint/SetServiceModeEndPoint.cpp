#include "SetServiceModeEndPoint.hpp"

#include "lifecycle/SetServiceModeRequest.hpp"

#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


SetServiceModeEndPoint::SetServiceModeEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new SetServiceModeRequestPubSubType()));

}

SetServiceModeEndPoint::~SetServiceModeEndPoint() {

}



void SetServiceModeEndPoint::OnDataAvailable(void* buffer){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (buffer != nullptr){

        SetServiceModeRequest* message = reinterpret_cast<SetServiceModeRequest*>(buffer);

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetState();

        if (manager != nullptr){
            manager->SetServiceMode(message);            
        }
    }

}

void* SetServiceModeEndPoint::GetTopicBuffer(){
    return &data_;
}


void SetServiceModeEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}


} // namespace lifecycle_manager
