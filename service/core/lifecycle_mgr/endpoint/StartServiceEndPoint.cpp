#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


StartServiceEndPoint::StartServiceEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new StartServiceRequestPubSubType()));

}

StartServiceEndPoint::~StartServiceEndPoint() {

}



void StartServiceEndPoint::OnDataAvailable(void* buffer){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (buffer != nullptr){

        StartServiceResponse* message = reinterpret_cast<StartServiceResponse*>(buffer);

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetState();

        if (manager != nullptr){
            manager->OnStart(message);            
        }

    }

}

void* StartServiceEndPoint::GetTopicBuffer(){
    return &resp_;
}


void StartServiceEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}

} // namespace lifecycle_manager
