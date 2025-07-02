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



void SetServiceModeEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<SetServiceModeRequest>(data_) == true){

        SetServiceModeRequest* message = reinterpret_cast<SetServiceModeRequest*>(&data_);

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetState();

        if (manager != nullptr){
            manager->SetServiceMode(message);            
        }
    }

}


void SetServiceModeEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}


} // namespace lifecycle_manager
