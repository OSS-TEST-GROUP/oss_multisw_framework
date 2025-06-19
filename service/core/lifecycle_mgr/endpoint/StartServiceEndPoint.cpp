#include "StartServiceEndPoint.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateManager.hpp"

namespace lifecycle_manager {


StartServiceEndPoint::StartServiceEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new StartServiceRequestPubSubType()));

}

StartServiceEndPoint::~StartServiceEndPoint() {

}



void StartServiceEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        StartServiceResponse* message = reinterpret_cast<StartServiceResponse*>(buffer);

        std::cout << "Message Delivery for " << GetTopic() << std::endl;

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

    std::cout << "Message Published " << GetTopic() << ", " << ret << std::endl;

}

} // namespace lifecycle_manager
