#include "SetServiceModeEndPoint.hpp"

#include "lifecycle/SetServiceModeRequest.hpp"

namespace lifecycle_manager {


SetServiceModeEndPoint::SetServiceModeEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new SetServiceModeRequestPubSubType()));

}

SetServiceModeEndPoint::~SetServiceModeEndPoint() {

}



void SetServiceModeEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        SetServiceModeRequest* message = reinterpret_cast<SetServiceModeRequest*>(buffer);

        std::cout << "Message Delivery for " << GetTopic() << std::endl;

        std::cout << "count : " << message->service_list().size() << std::endl;

    }

}

void* SetServiceModeEndPoint::GetTopicBuffer(){
    return &data_;
}


} // namespace lifecycle_manager
