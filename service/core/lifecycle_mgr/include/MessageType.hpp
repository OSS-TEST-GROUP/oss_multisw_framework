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

    // ── F013: Container management (lifecycle_mgr ↔ adsw_manager) ─────
    NOTIFY_CONTAINER_STATE,         // adsw_manager → lifecycle_mgr (sub)
    START_CONTAINER_REQUEST,        // lifecycle_mgr → adsw_manager  (pub)
    START_CONTAINER_RESPONSE,       // adsw_manager → lifecycle_mgr  (sub)
    STOP_CONTAINER_REQUEST,         // lifecycle_mgr → adsw_manager  (pub)
    STOP_CONTAINER_RESPONSE,        // adsw_manager → lifecycle_mgr  (sub)

    // ── F013: Operator/monitor → lifecycle_mgr policy gate (ctrl/*) ──
    // lifecycle_mgr subscribes for every request, gates, then forwards to
    // the actuator topics above (or to existing Start/Stop/SetServiceMode
    // request topics).  CtrlResponse is the shared response shape echoed
    // back to the client on the matching ctrl/*Response topic.
    CTRL_START_SERVICE_REQUEST,
    CTRL_START_SERVICE_RESPONSE,
    CTRL_STOP_SERVICE_REQUEST,
    CTRL_STOP_SERVICE_RESPONSE,
    CTRL_SET_SERVICE_MODE_REQUEST,
    CTRL_SET_SERVICE_MODE_RESPONSE,
    CTRL_START_CONTAINER_REQUEST,
    CTRL_START_CONTAINER_RESPONSE,
    CTRL_STOP_CONTAINER_REQUEST,
    CTRL_STOP_CONTAINER_RESPONSE,
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

            // F013: container management
            case MessageType::NOTIFY_CONTAINER_STATE   : return "mmf/lifecyclemgr/NotifyContainerState";
            case MessageType::START_CONTAINER_REQUEST  : return "mmf/lifecyclemgr/StartContainerRequest";
            case MessageType::START_CONTAINER_RESPONSE : return "mmf/lifecyclemgr/StartContainerResponse";
            case MessageType::STOP_CONTAINER_REQUEST   : return "mmf/lifecyclemgr/StopContainerRequest";
            case MessageType::STOP_CONTAINER_RESPONSE  : return "mmf/lifecyclemgr/StopContainerResponse";

            // F013: ctrl/* — monitor → lifecycle_mgr command gate
            case MessageType::CTRL_START_SERVICE_REQUEST     : return "mmf/lifecyclemgr/ctrl/StartService";
            case MessageType::CTRL_START_SERVICE_RESPONSE    : return "mmf/lifecyclemgr/ctrl/StartServiceResponse";
            case MessageType::CTRL_STOP_SERVICE_REQUEST      : return "mmf/lifecyclemgr/ctrl/StopService";
            case MessageType::CTRL_STOP_SERVICE_RESPONSE     : return "mmf/lifecyclemgr/ctrl/StopServiceResponse";
            case MessageType::CTRL_SET_SERVICE_MODE_REQUEST  : return "mmf/lifecyclemgr/ctrl/SetServiceMode";
            case MessageType::CTRL_SET_SERVICE_MODE_RESPONSE : return "mmf/lifecyclemgr/ctrl/SetServiceModeResponse";
            case MessageType::CTRL_START_CONTAINER_REQUEST   : return "mmf/lifecyclemgr/ctrl/StartContainer";
            case MessageType::CTRL_START_CONTAINER_RESPONSE  : return "mmf/lifecyclemgr/ctrl/StartContainerResponse";
            case MessageType::CTRL_STOP_CONTAINER_REQUEST    : return "mmf/lifecyclemgr/ctrl/StopContainer";
            case MessageType::CTRL_STOP_CONTAINER_RESPONSE   : return "mmf/lifecyclemgr/ctrl/StopContainerResponse";
        }
        return "";
    }
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_TYPE_HPP_
