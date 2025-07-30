#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


StartServiceEndPoint::StartServiceEndPoint(MessageType msgtype, std::string topic) 
    : DDSMessageEndPoint(topic){
        
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

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetStateMgr();

        if (manager != nullptr){
            manager->OnStart(message);            
        }
    }

}

void StartServiceEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
