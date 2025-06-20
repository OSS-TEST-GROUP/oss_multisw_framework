#include "UpdateServiceListEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

#include "manager/LifecycleFactory.hpp"
#include "service/ServiceRegistry.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


UpdateServiceListEndPoint::UpdateServiceListEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new UpdateServiceListRequestPubSubType()));

}

UpdateServiceListEndPoint::~UpdateServiceListEndPoint() {

}


void UpdateServiceListEndPoint::OnDataAvailable(void* buffer){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (buffer != nullptr){

        UpdateServiceListRequest* message = reinterpret_cast<UpdateServiceListRequest*>(buffer);

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

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << (ret == true) ? "Success" : "Fail");
}



} // namespace lifecycle_manager
