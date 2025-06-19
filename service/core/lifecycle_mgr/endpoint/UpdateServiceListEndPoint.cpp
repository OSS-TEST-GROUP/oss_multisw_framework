#include "UpdateServiceListEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

#include "manager/LifecycleFactory.hpp"
#include "service/ServiceRegistry.hpp"

namespace lifecycle_manager {


UpdateServiceListEndPoint::UpdateServiceListEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType()));

}

UpdateServiceListEndPoint::~UpdateServiceListEndPoint() {

}



void UpdateServiceListEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        UpdateServiceListRequest* message = reinterpret_cast<UpdateServiceListRequest*>(buffer);

        std::cout << "Message Delivery for " << GetTopic() << std::endl;
        std::cout << "count : " << message->service_list().size() << std::endl;

        std::shared_ptr<ServiceRegistry> manager = LifecycleFactory::GetInstance()->GetRegistry();

        if (manager != nullptr){
            manager->UpdateServiceList(message);            
        }
    }
}

void* UpdateServiceListEndPoint::GetTopicBuffer(){
    return &data_;
}


void UpdateServiceListEndPoint::OnDataPublished(bool ret){

    std::cout << "Message Published " << GetTopic() << ", " << ret << std::endl;

}



} // namespace lifecycle_manager
