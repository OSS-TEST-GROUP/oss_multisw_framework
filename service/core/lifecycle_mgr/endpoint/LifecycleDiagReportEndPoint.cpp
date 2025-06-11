#include "LifecycleDiagReportEndPoint.hpp"

#include "lifecycle/LifecycleDiagReport.hpp"

namespace lifecycle_manager {


LifecycleDiagReportEndPoint::LifecycleDiagReportEndPoint(std::string topic) 
    : DDSMessageEndPoint(topic){

    SetTypeSupport(std::make_shared<TypeSupport>(new LifecycleDiagReportPubSubType()));

}

LifecycleDiagReportEndPoint::~LifecycleDiagReportEndPoint() {

}



void LifecycleDiagReportEndPoint::OnDataAvailable(void* buffer){

    if (buffer != nullptr){

        LifecycleDiagReport* message = reinterpret_cast<LifecycleDiagReport*>(buffer);

        std::cout << "Message Delivery for " << GetTopic() << std::endl;

        std::cout << "count : " << message->service_list().size() << std::endl;
       
        std::vector<::ServiceStatus> *slist = &message->service_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            std::cout << "service " << svc->name() 
                        << ", " << static_cast<int32_t>(svc->service_type()) 
                        << ", " << static_cast<int32_t>(svc->service_state()) 
                        << ", " << static_cast<bool>(svc->is_enabled()) << std::endl;

            //GetServieManager()->UpdateService();
        }
    }

}

void* LifecycleDiagReportEndPoint::GetTopicBuffer(){
    return &data_;
}


} // namespace lifecycle_manager
