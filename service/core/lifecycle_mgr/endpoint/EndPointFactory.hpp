#ifndef LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "transmitter/DDSTransmitter.hpp"
#include "include/MessageType.hpp"

// Forward declarations for IDL message types used in EndPointHandlers
namespace lifecyclemgr {
    class StartServiceResponse;
    class StopServiceResponse;
    class SetServiceModeRequest;
    class GetServiceStateResponse;
    class NotifyServiceState;
    class UpdateServiceListRequest;
    // F013
    class NotifyContainerState;
    class StartContainerResponse;
    class StopContainerResponse;
    class CtrlStartServiceRequest;
    class CtrlStopServiceRequest;
    class CtrlSetServiceModeRequest;
    class CtrlStartContainerRequest;
    class CtrlStopContainerRequest;
    // F014 follow-up: recovery policy mutation
    class SetRecoveryPolicyResponse;
    class CtrlSetRecoveryPolicyRequest;
}

namespace lifecycle_manager {

struct EndPointHandlers {
    std::function<void(lifecyclemgr::StartServiceResponse*)>     on_start;
    std::function<void(lifecyclemgr::StopServiceResponse*)>      on_stop;
    std::function<void(lifecyclemgr::SetServiceModeRequest*)>    on_set_mode;
    std::function<void(lifecyclemgr::GetServiceStateResponse*)>  on_get_state_resp;
    std::function<void(lifecyclemgr::NotifyServiceState*)>       on_notify_state;
    std::function<void(lifecyclemgr::UpdateServiceListRequest*)> on_update_list;

    // F013: Container actuators — sub callbacks for messages from adsw_manager.
    std::function<void(lifecyclemgr::NotifyContainerState*)>        on_notify_container_state;
    std::function<void(lifecyclemgr::StartContainerResponse*)>      on_start_container_resp;
    std::function<void(lifecyclemgr::StopContainerResponse*)>       on_stop_container_resp;

    // F013: ctrl/* gate — sub callbacks for requests from operator/monitor.
    std::function<void(lifecyclemgr::CtrlStartServiceRequest*)>     on_ctrl_start_service;
    std::function<void(lifecyclemgr::CtrlStopServiceRequest*)>      on_ctrl_stop_service;
    std::function<void(lifecyclemgr::CtrlSetServiceModeRequest*)>   on_ctrl_set_service_mode;
    std::function<void(lifecyclemgr::CtrlStartContainerRequest*)>   on_ctrl_start_container;
    std::function<void(lifecyclemgr::CtrlStopContainerRequest*)>    on_ctrl_stop_container;

    // F014 follow-up: recovery policy mutation
    std::function<void(lifecyclemgr::SetRecoveryPolicyResponse*)>     on_set_recovery_policy_resp;
    std::function<void(lifecyclemgr::CtrlSetRecoveryPolicyRequest*)>  on_ctrl_set_recovery_policy;
};

class EndPointFactory  {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();

    int32_t Initialize();

    int32_t CreateEndPoints(const std::shared_ptr<DDSTransmitter>& transmitter,
                            const EndPointHandlers& handlers = EndPointHandlers{});

    const char* GetTopic(MessageType mtype);

    // Allow injection of custom endpoint creators for testing
    using EndpointCreator = std::function<std::shared_ptr<DDSMessageEndPoint>(MessageType, const std::string&)>;
    void SetCustomCreator(MessageType type, EndpointCreator creator) {
        custom_creators_[type] = std::move(std::move(creator));
    }

  private:
    std::map<MessageType, std::string> topic_lists_;
    std::map<MessageType, EndpointCreator> custom_creators_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_ENDPOINT_FACTORY_HPP_
