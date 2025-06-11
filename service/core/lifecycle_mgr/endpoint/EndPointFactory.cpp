#include "EndPointFactory.hpp"

#include "LifecycleDiagReportEndPoint.hpp"
#include "SetServiceModeEndPoint.hpp"
#include "UpdateServiceListEndPoint.hpp"


namespace lifecycle_manager {


constexpr const char* const topic_diag_report = "mmf/lifecyclemgr/LifecycleDiagReport";

constexpr const char* const topic_set_svc_req = "mmf/lifecyclemgr/SetServiceModeRequest";
constexpr const char* const topic_set_svc_resp = "mmf/lifecyclemgr/SetServiceModeResponse";

constexpr const char* const topic_update_svc_req = "mmf/lifecyclemgr/UpdateServiceListRequest";
constexpr const char* const topic_update_svc_resp = "mmf/lifecyclemgr/UpdateServiceListResponse";

constexpr const char* const topic_type = "lifecyclemgr";

EndPointFactory::EndPointFactory(){
}

EndPointFactory::~EndPointFactory() {

}



int32_t EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter){

    std::shared_ptr<LifecycleDiagReportEndPoint> diag_report 
            = std::make_shared<LifecycleDiagReportEndPoint>(topic_diag_report);


    DomainParticipant* participant = transmitter->GetDomain();
    Publisher* publisher = transmitter->GetPublisher();
    Subscriber* subscriber = transmitter->GetSubscriber();

    std::cout << __FUNCTION__ << "::" << __LINE__ << std::endl;
    if (diag_report){
        
        diag_report->Initialize(participant, publisher, subscriber);
        transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndPoint>(diag_report));
    }

    std::shared_ptr<SetServiceModeEndPoint> set_svc_mode 
            = std::make_shared<SetServiceModeEndPoint>(topic_set_svc_req);
    if (set_svc_mode){

        set_svc_mode->Initialize(participant, publisher, subscriber);
        transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndPoint>(set_svc_mode));
    }            

    std::shared_ptr<UpdateServiceListEndPoint> update_svc 
            = std::make_shared<UpdateServiceListEndPoint>(topic_update_svc_req);
    if (update_svc){

        update_svc->Initialize(participant, publisher, subscriber);
        transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndPoint>(update_svc));
    }

    return 3;

}


} // namespace lifecycle_manager
