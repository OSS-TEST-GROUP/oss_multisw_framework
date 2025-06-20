#include "NotifyServiceStateEndPoint.hpp"

#include "lifecycle/NotifyServiceState.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType()));

}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}



void NotifyServiceStateEndPoint::OnDataAvailable(void* buffer){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (buffer != nullptr){

        NotifyServiceState* message = reinterpret_cast<NotifyServiceState*>(buffer);

        std::shared_ptr<StateMonitor> manager = LifecycleFactory::GetInstance()->GetMonitor();

        if (manager != nullptr){
            manager->UpdateServicesState(message);            
        }
    }

}

void* NotifyServiceStateEndPoint::GetTopicBuffer(){
    return &data_;
}


void NotifyServiceStateEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}

} // namespace lifecycle_manager
