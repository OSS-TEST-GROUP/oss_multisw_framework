#include "GetServiceStateEndPoint.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"

namespace lifecycle_manager {


GetServiceStateEndPoint::GetServiceStateEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new GetServiceStateRequestPubSubType()));

}

GetServiceStateEndPoint::~GetServiceStateEndPoint() {

}



void GetServiceStateEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        GetServiceStateResponse* response = reinterpret_cast<GetServiceStateResponse*>(buffer);

        std::shared_ptr<StateMonitor> manager = LifecycleFactory::GetInstance()->GetMonitor();

        if (manager != nullptr){
            manager->UpdateServicesState(response);            
        }

        std::cout << "Message Delivery for " << GetTopic() << std::endl;
    }

}

void* GetServiceStateEndPoint::GetTopicBuffer(){
    return &resp_;
}


void GetServiceStateEndPoint::OnDataPublished(bool ret){

    std::cout << "Message Published " << GetTopic() << ", " << ret << std::endl;

}

} // namespace lifecycle_manager
