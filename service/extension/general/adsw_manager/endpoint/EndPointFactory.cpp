
#include "EndPointFactory.hpp"
#include "logger/Log.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "GetServiceStateEndPoint.hpp"
#include "NotifyServiceStateEndPoint.hpp"
#include "StartServiceEndPoint.hpp"
#include "StopServiceEndPoint.hpp"
#include "UpdateServiceListEndPoint.hpp"
#include "AdswServiceStatusEndPoint.hpp"

// F013
#include "NotifyContainerStateEndPoint.hpp"
#include "StartContainerEndPoint.hpp"
#include "StopContainerEndPoint.hpp"
// F014 follow-up
#include "SetRecoveryPolicyEndPoint.hpp"

namespace general {
namespace adsw_manager {

EndPointFactory::EndPointFactory() {
}
EndPointFactory::~EndPointFactory() {
}

bool EndPointFactory::Initialize(){
    topic_lists_[MessageType::GET_SERVICE_STATE_REQUEST] = "mmf/lifecyclemgr/GetServiceStateRequest";
    topic_lists_[MessageType::GET_SERVICE_STATE_RESPONSE] = "mmf/lifecyclemgr/GetServiceStateResponse";
    topic_lists_[MessageType::NOTIFY_SERVICE_STATE] = "mmf/lifecyclemgr/NotifyServiceState";
    topic_lists_[MessageType::START_SERVICE_REQUEST] = "mmf/lifecyclemgr/StartServiceRequest";
    topic_lists_[MessageType::START_SERVICE_RESPONSE] = "mmf/lifecyclemgr/StartServiceResponse";
    topic_lists_[MessageType::STOP_SERVICE_REQUEST] = "mmf/lifecyclemgr/StopServiceRequest";
    topic_lists_[MessageType::STOP_SERVICE_RESPONSE] = "mmf/lifecyclemgr/StopServiceResponse";
    topic_lists_[MessageType::UPDATE_SERVICE_LIST_REQUEST] = "mmf/lifecyclemgr/UpdateServiceListRequest";
    topic_lists_[MessageType::UPDATE_SERVICE_LIST_RESPONSE] = "mmf/lifecyclemgr/UpdateServiceListResponse";
    topic_lists_[MessageType::ADSW_SERVICE_STATUS] = "mmf/adswmgr/AdswServiceStatus";

    // F013: container actuator surface (paired with lifecycle_mgr S2b topics)
    topic_lists_[MessageType::NOTIFY_CONTAINER_STATE]   = "mmf/lifecyclemgr/NotifyContainerState";
    topic_lists_[MessageType::START_CONTAINER_REQUEST]  = "mmf/lifecyclemgr/StartContainerRequest";
    topic_lists_[MessageType::START_CONTAINER_RESPONSE] = "mmf/lifecyclemgr/StartContainerResponse";
    topic_lists_[MessageType::STOP_CONTAINER_REQUEST]   = "mmf/lifecyclemgr/StopContainerRequest";
    topic_lists_[MessageType::STOP_CONTAINER_RESPONSE]  = "mmf/lifecyclemgr/StopContainerResponse";

    // F014 follow-up: recovery policy actuator
    topic_lists_[MessageType::SET_RECOVERY_POLICY_REQUEST]  = "mmf/lifecyclemgr/SetRecoveryPolicyRequest";
    topic_lists_[MessageType::SET_RECOVERY_POLICY_RESPONSE] = "mmf/lifecyclemgr/SetRecoveryPolicyResponse";

    return true;
}


void EndPointFactory::Finalize(){
    topic_lists_.clear();    
}

int32_t EndPointFactory::CreateEndPoints(const std::shared_ptr<DDSTransmitter>& transmitter) {

    DomainParticipant* participant = transmitter->GetDomain();
    Publisher* publisher = transmitter->GetPublisher();
    Subscriber* subscriber = transmitter->GetSubscriber();


    for (auto const& [key, val] : topic_lists_)
    {
        LOG_DEBUG(__FUNCTION__ << "::" << static_cast<int32_t>(key) << "," << val);

        std::shared_ptr<DDSMessageEndpoint> msg_endpoint = nullptr;
        PubSubType pubsub = PubSubType::PUBSUB_NONE;

        switch(key){
            case MessageType::GET_SERVICE_STATE_REQUEST :{
                std::shared_ptr<GetServiceStateEndPoint> get_service_state = std::make_shared<GetServiceStateEndPoint>(key, val);

                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(get_service_state);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;
            case MessageType::GET_SERVICE_STATE_RESPONSE :{
                std::shared_ptr<GetServiceStateEndPoint> get_service_state = std::make_shared<GetServiceStateEndPoint>(key, val);

                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(get_service_state);
                pubsub = PubSubType::PUBLISH_ONLY;
            } 
            break;

            case MessageType::NOTIFY_SERVICE_STATE :{
                std::shared_ptr<NotifyServiceStateEndPoint> notify_service_state = std::make_shared<NotifyServiceStateEndPoint>(key, val);

                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(notify_service_state);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::START_SERVICE_REQUEST :{     
                std::shared_ptr<StartServiceEndPoint> start_service = std::make_shared<StartServiceEndPoint>(key, val);  
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(start_service);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;
            case MessageType::START_SERVICE_RESPONSE :{ 
                std::shared_ptr<StartServiceEndPoint> start_service = std::make_shared<StartServiceEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(start_service);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::STOP_SERVICE_REQUEST :{
                std::shared_ptr<StopServiceEndPoint> stop_service = std::make_shared<StopServiceEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(stop_service);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;

            case MessageType::STOP_SERVICE_RESPONSE :{
                std::shared_ptr<StopServiceEndPoint> stop_service = std::make_shared<StopServiceEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(stop_service);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::UPDATE_SERVICE_LIST_REQUEST :{    
                std::shared_ptr<UpdateServiceListEndPoint> update_service_list = std::make_shared<UpdateServiceListEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(update_service_list);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::UPDATE_SERVICE_LIST_RESPONSE :{
                std::shared_ptr<UpdateServiceListEndPoint> update_service_list = std::make_shared<UpdateServiceListEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(update_service_list);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;

            case MessageType::ADSW_SERVICE_STATUS :{
                std::shared_ptr<ADSWServiceStatusEndPoint> adsw_service_status = std::make_shared<ADSWServiceStatusEndPoint>(key, val);

                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(adsw_service_status);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            // F013: container actuator surface
            case MessageType::NOTIFY_CONTAINER_STATE :{
                auto ep = std::make_shared<NotifyContainerStateEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::START_CONTAINER_REQUEST :{
                auto ep = std::make_shared<StartContainerEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;

            case MessageType::START_CONTAINER_RESPONSE :{
                auto ep = std::make_shared<StartContainerEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            case MessageType::STOP_CONTAINER_REQUEST :{
                auto ep = std::make_shared<StopContainerEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;

            case MessageType::STOP_CONTAINER_RESPONSE :{
                auto ep = std::make_shared<StopContainerEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            // F014 follow-up: recovery policy actuator
            case MessageType::SET_RECOVERY_POLICY_REQUEST :{
                auto ep = std::make_shared<SetRecoveryPolicyEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::SUBSCRIBE_ONLY;
            }
            break;

            case MessageType::SET_RECOVERY_POLICY_RESPONSE :{
                auto ep = std::make_shared<SetRecoveryPolicyEndPoint>(key, val);
                msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndpoint>(ep);
                pubsub = PubSubType::PUBLISH_ONLY;
            }
            break;

            default:
                LOG_ERROR("Unknown message type: " << static_cast<int32_t>(key));
                continue;
        }

        if (msg_endpoint){
            if (pubsub == PubSubType::PUBLISH_ONLY){
                msg_endpoint->Initialize(participant, publisher, nullptr);
            }else if (pubsub == PubSubType::SUBSCRIBE_ONLY){
                msg_endpoint->Initialize(participant, nullptr, subscriber);
            }else if (pubsub == PubSubType::PUBSUB_BOTH){
                msg_endpoint->Initialize(participant, publisher, subscriber);   
            }
            
            transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndpoint>(msg_endpoint));
        }
    }

    // 토픽 표는 MessageType enum 크기로 상한이 잡힌다. 축소를 명시한다.
    return static_cast<int32_t>(topic_lists_.size());
}

const char* EndPointFactory::GetTopic(MessageType mtype){

    auto it = topic_lists_.find(mtype);
    if (it != topic_lists_.end()) {
        return it->second.c_str();
    }

    return nullptr;
}


} // namespace adsw_manager
} // namespace general
