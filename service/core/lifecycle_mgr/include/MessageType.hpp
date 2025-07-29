#ifndef LIFECYCLE_MESSAGE_TYPE_HPP_
#define LIFECYCLE_MESSAGE_TYPE_HPP_

namespace lifecycle_manager {

enum class MessageType {
    LIFECYCLE_DIAG_REPORT,
    GET_SERVICE_STATE_REQUEST,
    GET_SERVICE_STATE_RESPONSE,
    NOTIFY_SERVICE_STATE,

    SET_SERVICE_MODE_REQUST,
    SET_SERVICE_MODE_RESPONSE,

    START_SERVICE_REQUEST,
    START_SERVICE_RESPONSE,

    STOP_SERVICE_REQUEST,
    STOP_SERVICE_RESPONSE,

    UPDATE_SERVICE_LIST_REQUEST,
    UPDATE_SERVICE_LIST_RESPONSE,

};


enum class PubSubTYpe {
    PUBSUB_NONE,
    PUBLISH_ONLY,
    SUBSCRIBE_ONLY,

    PUBSUB_BOTH,
};


class MessageTopic {
  public:
    MessageTopic();
    virtual ~MessageTopic();

    static std::string GetTopicStr(MessageType type){

        switch (type){
            case MessageType::LIFECYCLE_DIAG_REPORT : return "mmf/lifecyclemgr/LifecycleDiagReport";

            case MessageType::GET_SERVICE_STATE_REQUEST : return "mmf/lifecyclemgr/GetServiceStateRequest";
            case MessageType::GET_SERVICE_STATE_RESPONSE : return "mmf/lifecyclemgr/GetServiceStateResponse";

            case MessageType::NOTIFY_SERVICE_STATE : return "mmf/lifecyclemgr/NotifyServiceState";

            case MessageType::SET_SERVICE_MODE_REQUST : return "mmf/lifecyclemgr/SetServiceModeRequest";
            case MessageType::SET_SERVICE_MODE_RESPONSE : return "mmf/lifecyclemgr/SetServiceModeResponse";

            case MessageType::START_SERVICE_REQUEST : return "mmf/lifecyclemgr/StartServiceRequest";
            case MessageType::START_SERVICE_RESPONSE : return "mmf/lifecyclemgr/StartServiceResponse";

            case MessageType::STOP_SERVICE_REQUEST : return "mmf/lifecyclemgr/StopServiceRequest";
            case MessageType::STOP_SERVICE_RESPONSE : return "mmf/lifecyclemgr/StopServiceResponse";

            case MessageType::UPDATE_SERVICE_LIST_REQUEST : return "mmf/lifecyclemgr/UpdateServiceListRequest";
            case MessageType::UPDATE_SERVICE_LIST_RESPONSE : return "mmf/lifecyclemgr/UpdateServiceListResponse";

        }
        return "";
    }
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_TYPE_HPP_
