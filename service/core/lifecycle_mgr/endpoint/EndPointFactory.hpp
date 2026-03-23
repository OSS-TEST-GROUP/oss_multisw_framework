#ifndef LIFECYCLE_ENDPOINT_FACTORY_HPP_
#define LIFECYCLE_ENDPOINT_FACTORY_HPP_

#include <string>
#include <map>
#include <memory>
#include <functional>

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
}

namespace lifecycle_manager {

struct EndPointHandlers {
    std::function<void(lifecyclemgr::StartServiceResponse*)>     on_start;
    std::function<void(lifecyclemgr::StopServiceResponse*)>      on_stop;
    std::function<void(lifecyclemgr::SetServiceModeRequest*)>    on_set_mode;
    std::function<void(lifecyclemgr::GetServiceStateResponse*)>  on_get_state_resp;
    std::function<void(lifecyclemgr::NotifyServiceState*)>       on_notify_state;
    std::function<void(lifecyclemgr::UpdateServiceListRequest*)> on_update_list;
};

class EndPointFactory  {
  public:
    EndPointFactory();
    virtual ~EndPointFactory();

    int32_t Initialize();

    int32_t CreateEndPoints(std::shared_ptr<DDSTransmitter> transmitter,
                            const EndPointHandlers& handlers = EndPointHandlers{});

    const char* GetTopic(MessageType mtype);

    // Allow injection of custom endpoint creators for testing
    using EndpointCreator = std::function<std::shared_ptr<DDSMessageEndPoint>(MessageType, const std::string&)>;
    void SetCustomCreator(MessageType type, EndpointCreator creator) {
        custom_creators_[type] = creator;
    }

  private:
    std::map<MessageType, std::string> topic_lists_;
    std::map<MessageType, EndpointCreator> custom_creators_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_ENDPOINT_FACTORY_HPP_
