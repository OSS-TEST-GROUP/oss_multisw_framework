#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


GetServiceStateEndPoint::GetServiceStateEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType()));

}

GetServiceStateEndPoint::~GetServiceStateEndPoint() {

}


void GetServiceStateEndPoint::OnDataAvailable(){
    
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<GetServiceStateResponse>(resp_) == true){

        GetServiceStateResponse* response = reinterpret_cast<GetServiceStateResponse*>(&resp_);

        std::shared_ptr<StateMonitor> manager = LifecycleFactory::GetInstance()->GetMonitor();

        if (manager != nullptr){
            manager->UpdateServicesState(response);            
        }

     }

}

void GetServiceStateEndPoint::OnDataPublished(bool ret){
    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
