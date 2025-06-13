#include "StopServiceEndPoint.hpp"

#include "lifecycle/StopServiceRequest.hpp"

namespace lifecycle_manager {


StopServiceEndPoint::StopServiceEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new StopServiceRequestPubSubType()));

}

StopServiceEndPoint::~StopServiceEndPoint() {

}



void StopServiceEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        StopServiceResponse* message = reinterpret_cast<StopServiceResponse*>(buffer);

        std::cout << "Message Delivery for " << GetTopic() << std::endl;
    }

}

void* StopServiceEndPoint::GetTopicBuffer(){
    return &resp_;
}


void StopServiceEndPoint::OnDataPublished(bool ret){

    std::cout << "Message Published " << GetTopic() << ", " << ret << std::endl;

}

} // namespace lifecycle_manager
