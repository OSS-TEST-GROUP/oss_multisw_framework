#include "NotifyServiceStateEndPoint.hpp"

#include "lifecycle/NotifyServiceState.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"

namespace lifecycle_manager {


NotifyServiceStateEndPoint::NotifyServiceStateEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new NotifyServiceStatePubSubType()));

}

NotifyServiceStateEndPoint::~NotifyServiceStateEndPoint() {

}



void NotifyServiceStateEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        NotifyServiceState* message = reinterpret_cast<NotifyServiceState*>(buffer);

        std::shared_ptr<StateMonitor> manager = LifecycleFactory::GetInstance()->GetMonitor();

        if (manager != nullptr){
            manager->UpdateServicesState(message);            
        }

        std::cout << "Message Delivery for " << GetTopic() << std::endl;
    }

}

void* NotifyServiceStateEndPoint::GetTopicBuffer(){
    return &data_;
}


void NotifyServiceStateEndPoint::OnDataPublished(bool ret){

    std::cout << "Message Published " << GetTopic() << ", " << ret << std::endl;

}

} // namespace lifecycle_manager
