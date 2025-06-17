#include "UpdateServiceListEndPoint.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

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

        std::vector<::Service> *slist = &message->service_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            std::cout << "service " << svc->name() 
                        << ", " << static_cast<int32_t>(svc->service_type()) 
                        << ", " << static_cast<int32_t>(svc->recovery_policy()) 
                        << ", " << static_cast<bool>(svc->is_enabled()) << std::endl;

            //GetServieManager()->UpdateService(svc->name(), svc->service_type(), );
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
