#include "EndPointFactory.hpp"

#include "LifecycleDiagReportEndPoint.hpp"
#include "SetServiceModeEndPoint.hpp"
#include "UpdateServiceListEndPoint.hpp"

#include "StartServiceEndPoint.hpp"
#include "StopServiceEndPoint.hpp"
#include "GetServiceStateEndPoint.hpp"
#include "NotifyServiceStateEndPoint.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {


constexpr const char* const topic_diag_report = "mmf/lifecyclemgr/LifecycleDiagReport";

constexpr const char* const topic_set_svc_req = "mmf/lifecyclemgr/SetServiceModeRequest";
constexpr const char* const topic_set_svc_resp = "mmf/lifecyclemgr/SetServiceModeResponse";

constexpr const char* const topic_update_svc_req = "mmf/lifecyclemgr/UpdateServiceListRequest";
constexpr const char* const topic_update_svc_resp = "mmf/lifecyclemgr/UpdateServiceListResponse";

constexpr const char* const topic_start_svc_req = "mmf/lifecyclemgr/StartServiceRequest";
constexpr const char* const topic_start_svc_resp = "mmf/lifecyclemgr/StartServiceResponse";

constexpr const char* const topic_stop_svc_req = "mmf/lifecyclemgr/StopServiceRequest";
constexpr const char* const topic_stop_svc_resp = "mmf/lifecyclemgr/StopServiceResponse";

constexpr const char* const topic_get_svc_state_req = "mmf/lifecyclemgr/GetServiceStateRequest";
constexpr const char* const topic_get_svc_state_resp = "mmf/lifecyclemgr/GetServiceStateResponse";

constexpr const char* const topic_notify_svc_state = "mmf/lifecyclemgr/NotifyServiceState";

constexpr const char* const topic_type = "lifecyclemgr";

using namespace lifecyclemgr;

EndPointFactory::EndPointFactory(){
}

EndPointFactory::~EndPointFactory() {

}

int32_t EndPointFactory::Initialize(){

    topic_lists_[MessageType::LIFECYCLE_DIAG_REPORT] = MessageTopic::GetTopicStr(MessageType::LIFECYCLE_DIAG_REPORT);

    topic_lists_[MessageType::GET_SERVICE_STATE_REQUEST] = MessageTopic::GetTopicStr(MessageType::GET_SERVICE_STATE_REQUEST);;
    topic_lists_[MessageType::GET_SERVICE_STATE_RESPONSE] = MessageTopic::GetTopicStr(MessageType::GET_SERVICE_STATE_RESPONSE);;
    topic_lists_[MessageType::NOTIFY_SERVICE_STATE] = MessageTopic::GetTopicStr(MessageType::NOTIFY_SERVICE_STATE);;

    topic_lists_[MessageType::SET_SERVICE_MODE_REQUST] = MessageTopic::GetTopicStr(MessageType::SET_SERVICE_MODE_REQUST);;
    topic_lists_[MessageType::SET_SERVICE_MODE_RESPONSE] = MessageTopic::GetTopicStr(MessageType::SET_SERVICE_MODE_RESPONSE);;

    topic_lists_[MessageType::START_SERVICE_REQUEST] = MessageTopic::GetTopicStr(MessageType::START_SERVICE_REQUEST);;
    topic_lists_[MessageType::START_SERVICE_RESPONSE] = MessageTopic::GetTopicStr(MessageType::START_SERVICE_RESPONSE);;

    topic_lists_[MessageType::STOP_SERVICE_REQUEST] = MessageTopic::GetTopicStr(MessageType::STOP_SERVICE_REQUEST);;
    topic_lists_[MessageType::STOP_SERVICE_RESPONSE] = MessageTopic::GetTopicStr(MessageType::STOP_SERVICE_RESPONSE);;

    topic_lists_[MessageType::UPDATE_SERVICE_LIST_REQUEST] = MessageTopic::GetTopicStr(MessageType::UPDATE_SERVICE_LIST_REQUEST);;
    topic_lists_[MessageType::UPDATE_SERVICE_LIST_RESPONSE] = MessageTopic::GetTopicStr(MessageType::UPDATE_SERVICE_LIST_RESPONSE);;

    // F013: Container management + ctrl/* surface. Topic-string registration
    // only at this stage — actual endpoint instantiation lives in the
    // CreateEndPoints() switch (added incrementally in S2b-3).  Unhandled
    // MessageTypes fall through with pubsub = PUBSUB_NONE so registering
    // them here is a safe no-op until the switch case lands.
    topic_lists_[MessageType::NOTIFY_CONTAINER_STATE]   = MessageTopic::GetTopicStr(MessageType::NOTIFY_CONTAINER_STATE);
    topic_lists_[MessageType::START_CONTAINER_REQUEST]  = MessageTopic::GetTopicStr(MessageType::START_CONTAINER_REQUEST);
    topic_lists_[MessageType::START_CONTAINER_RESPONSE] = MessageTopic::GetTopicStr(MessageType::START_CONTAINER_RESPONSE);
    topic_lists_[MessageType::STOP_CONTAINER_REQUEST]   = MessageTopic::GetTopicStr(MessageType::STOP_CONTAINER_REQUEST);
    topic_lists_[MessageType::STOP_CONTAINER_RESPONSE]  = MessageTopic::GetTopicStr(MessageType::STOP_CONTAINER_RESPONSE);

    topic_lists_[MessageType::CTRL_START_SERVICE_REQUEST]      = MessageTopic::GetTopicStr(MessageType::CTRL_START_SERVICE_REQUEST);
    topic_lists_[MessageType::CTRL_START_SERVICE_RESPONSE]     = MessageTopic::GetTopicStr(MessageType::CTRL_START_SERVICE_RESPONSE);
    topic_lists_[MessageType::CTRL_STOP_SERVICE_REQUEST]       = MessageTopic::GetTopicStr(MessageType::CTRL_STOP_SERVICE_REQUEST);
    topic_lists_[MessageType::CTRL_STOP_SERVICE_RESPONSE]      = MessageTopic::GetTopicStr(MessageType::CTRL_STOP_SERVICE_RESPONSE);
    topic_lists_[MessageType::CTRL_SET_SERVICE_MODE_REQUEST]   = MessageTopic::GetTopicStr(MessageType::CTRL_SET_SERVICE_MODE_REQUEST);
    topic_lists_[MessageType::CTRL_SET_SERVICE_MODE_RESPONSE]  = MessageTopic::GetTopicStr(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE);
    topic_lists_[MessageType::CTRL_START_CONTAINER_REQUEST]    = MessageTopic::GetTopicStr(MessageType::CTRL_START_CONTAINER_REQUEST);
    topic_lists_[MessageType::CTRL_START_CONTAINER_RESPONSE]   = MessageTopic::GetTopicStr(MessageType::CTRL_START_CONTAINER_RESPONSE);
    topic_lists_[MessageType::CTRL_STOP_CONTAINER_REQUEST]     = MessageTopic::GetTopicStr(MessageType::CTRL_STOP_CONTAINER_REQUEST);
    topic_lists_[MessageType::CTRL_STOP_CONTAINER_RESPONSE]    = MessageTopic::GetTopicStr(MessageType::CTRL_STOP_CONTAINER_RESPONSE);

    return topic_lists_.size();

}

int32_t EndPointFactory::CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter,
                                          const EndPointHandlers& handlers){


    DomainParticipant* participant = transmitter->GetDomain();
    Publisher* publisher = transmitter->GetPublisher();
    Subscriber* subscriber = transmitter->GetSubscriber();

    for (auto const& [key, val] : topic_lists_)
    {
        LOG_DEBUG(__FUNCTION__ << "::" << static_cast<int32_t>(key) << "," << val);

        std::shared_ptr<DDSMessageEndPoint> msg_endpoint = nullptr;
        PubSubTYpe pubsub = PubSubTYpe::PUBSUB_NONE;

        // Use custom creator if provided (for testing/mocking)
        auto custom_it = custom_creators_.find(key);
        if (custom_it != custom_creators_.end()) {
            msg_endpoint = custom_it->second(key, val);
            // For test, you may want to set pubsub as needed, or default to PUBSUB_BOTH
            pubsub = PubSubTYpe::PUBSUB_BOTH;
        } else {
            switch(key){
                case MessageType::LIFECYCLE_DIAG_REPORT :{
                    std::shared_ptr<LifecycleDiagReportEndPoint> diag_report = std::make_shared<LifecycleDiagReportEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(diag_report);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break;
                
                case MessageType::GET_SERVICE_STATE_REQUEST :{
                    std::shared_ptr<GetServiceStateEndPoint> get_svc_state_req = std::make_shared<GetServiceStateEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(get_svc_state_req);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break;     

                case MessageType::GET_SERVICE_STATE_RESPONSE :{
                    std::shared_ptr<GetServiceStateEndPoint> get_svc_state_resp = std::make_shared<GetServiceStateEndPoint>(key, val, handlers.on_get_state_resp);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(get_svc_state_resp);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break;     

                case MessageType::NOTIFY_SERVICE_STATE :{
                    std::shared_ptr<NotifyServiceStateEndPoint> notify_svc_state = std::make_shared<NotifyServiceStateEndPoint>(key, val, handlers.on_notify_state);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(notify_svc_state);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break;     

                case MessageType::SET_SERVICE_MODE_REQUST :{
                    std::shared_ptr<SetServiceModeEndPoint> set_svc_mode_req = std::make_shared<SetServiceModeEndPoint>(key, val, handlers.on_set_mode);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(set_svc_mode_req);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break;     

                case MessageType::SET_SERVICE_MODE_RESPONSE :{
                    std::shared_ptr<SetServiceModeEndPoint> set_svc_mode_resp = std::make_shared<SetServiceModeEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(set_svc_mode_resp);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break;        

                case MessageType::START_SERVICE_REQUEST :{
                    std::shared_ptr<StartServiceEndPoint> start_svc_req = std::make_shared<StartServiceEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(start_svc_req);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break; 

                case MessageType::START_SERVICE_RESPONSE :{
                    std::shared_ptr<StartServiceEndPoint> start_svc_resp = std::make_shared<StartServiceEndPoint>(key, val, handlers.on_start);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(start_svc_resp);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break; 

                case MessageType::STOP_SERVICE_REQUEST :{
                    std::shared_ptr<StopServiceEndPoint> stop_svc_req = std::make_shared<StopServiceEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(stop_svc_req);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break; 

                case MessageType::STOP_SERVICE_RESPONSE :{
                    std::shared_ptr<StopServiceEndPoint> stop_svc_resp = std::make_shared<StopServiceEndPoint>(key, val, handlers.on_stop);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(stop_svc_resp);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break;      

                case MessageType::UPDATE_SERVICE_LIST_REQUEST :{
                    std::shared_ptr<UpdateServiceListEndPoint> update_svc_req = std::make_shared<UpdateServiceListEndPoint>(key, val, handlers.on_update_list);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(update_svc_req);
                    pubsub = PubSubTYpe::SUBSCRIBE_ONLY;
                }
                break;                 

                case MessageType::UPDATE_SERVICE_LIST_RESPONSE :{
                    std::shared_ptr<UpdateServiceListEndPoint> update_svc_resp = std::make_shared<UpdateServiceListEndPoint>(key, val);

                    msg_endpoint = std::dynamic_pointer_cast<DDSMessageEndPoint>(update_svc_resp);
                    pubsub = PubSubTYpe::PUBLISH_ONLY;
                }
                break;  
            }
        }

        if (msg_endpoint){
            if (pubsub == PubSubTYpe::PUBLISH_ONLY){
                msg_endpoint->Initialize(participant, publisher, nullptr);
            }else if (pubsub == PubSubTYpe::SUBSCRIBE_ONLY){
                msg_endpoint->Initialize(participant, nullptr, subscriber);
            }else if (pubsub == PubSubTYpe::PUBSUB_BOTH){
                msg_endpoint->Initialize(participant, publisher, subscriber);   
            }
            
            transmitter->AddMessageEndpoint(std::dynamic_pointer_cast<DDSMessageEndPoint>(msg_endpoint));
        }

    }

    return topic_lists_.size();

}


const char* EndPointFactory::GetTopic(MessageType mtype){

    auto it = topic_lists_.find(mtype);
    if (it != topic_lists_.end()) {
        return it->second.c_str();
    }

    return nullptr;
}


} // namespace lifecycle_manager
