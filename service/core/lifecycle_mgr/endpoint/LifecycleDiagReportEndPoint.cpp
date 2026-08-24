#include "LifecycleDiagReportEndPoint.hpp"

#include <utility>

#include "lifecycle/LifecycleDiagReport.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

LifecycleDiagReportEndPoint::LifecycleDiagReportEndPoint(MessageType msgtype, std::string topic)
    : DDSMessageEndPoint(std::move(topic)) {

    if (msgtype == MessageType::LIFECYCLE_DIAG_REPORT){
        SetTypeSupport(std::make_shared<TypeSupport>(new LifecycleDiagReportPubSubType()));
    }
}

LifecycleDiagReportEndPoint::~LifecycleDiagReportEndPoint() {

}



void LifecycleDiagReportEndPoint::OnDataAvailable(){
    LOG_DEBUG("Message Delivery for " << GetTopic() );

    if (Read<LifecycleDiagReport>(data_)) {

        LifecycleDiagReport* message = reinterpret_cast<LifecycleDiagReport*>(&data_);

        std::vector<lifecyclemgr::ServiceStatus> *slist = &message->service_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            LOG_VERBOSE("service " << svc->name() 
                        << ", " << static_cast<int32_t>(svc->service_type()) 
                        << ", " << static_cast<int32_t>(svc->service_state()) 
                        << ", " << static_cast<bool>(svc->is_enabled()));
        }
    }
}

void LifecycleDiagReportEndPoint::OnDataPublished(bool ret){

    LOG_DEBUG("Message Published for " << GetTopic() << ", " << ((ret == true) ? "Success" : "Fail"));
}

} // namespace lifecycle_manager
