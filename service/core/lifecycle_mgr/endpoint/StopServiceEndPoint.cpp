#include "StopServiceEndPoint.hpp"

#include "lifecycle/StopServiceRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

StopServiceEndPoint::StopServiceEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new StopServiceRequestPubSubType()));

}

StopServiceEndPoint::~StopServiceEndPoint() {

}



void StopServiceEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<StopServiceResponse>(resp_) == true){

        StopServiceResponse* message = reinterpret_cast<StopServiceResponse*>(&resp_);

        std::shared_ptr<StateManager> manager = LifecycleFactory::GetInstance()->GetState();

        if (manager != nullptr){
            manager->OnStop(message);            
        }
    }

}


void StopServiceEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}

} // namespace lifecycle_manager
