#include "StartContainerEndPoint.hpp"

#include "lifecycle/StartContainerRequest.hpp"
#include "ADSWManager.hpp"
#include "control/ContainerController.hpp"
#include "control/ContainerServiceController.hpp"
#include "transmitter/DDSTransmitter.hpp"

#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

StartContainerEndPoint::StartContainerEndPoint(MessageType msgtype, std::string topic)
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::START_CONTAINER_REQUEST) {
        type_support_ = std::make_shared<TypeSupport>(new StartContainerRequestPubSubType());
    } else if (msgtype == MessageType::START_CONTAINER_RESPONSE) {
        type_support_ = std::make_shared<TypeSupport>(new StartContainerResponsePubSubType());
    }
}

StartContainerEndPoint::~StartContainerEndPoint() {}

std::string StartContainerEndPoint::GetTopic() { return topic_; }
std::string StartContainerEndPoint::GetTypeName() { return type_support_->get_type_name(); }
std::shared_ptr<TypeSupport> StartContainerEndPoint::GetTypeSupport() { return type_support_; }

void StartContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<StartContainerRequest>(req_) != true) return;

    StartContainerRequest* message = reinterpret_cast<StartContainerRequest*>(&req_);

    auto controller    = ADSWManager::Get() ? ADSWManager::Get()->GetServiceController() : nullptr;
    auto container_ctrl = controller ? controller->GetContainer() : nullptr;
    bool ok = (container_ctrl != nullptr);
    if (!ok) {
        LOG_ERROR("StartContainer: container controller unavailable");
    }

    // Iterate every requested container. The verdict is success only if
    // every StartContainer() call succeeded — partial failures still get a
    // single FAILED response so the monitor reflects the worst-case.
    for (const auto& name : message->container_list()) {
        if (!container_ctrl) { ok = false; break; }
        bool r = container_ctrl->StartContainer(name);
        LOG_DEBUG("StartContainer name=" << name << " result=" << (r ? "OK" : "FAIL"));
        if (!r) ok = false;
    }

    resp_.ret_code() = ok ? lifecyclemgr::ReturnCode::SUCCESS
                          : lifecyclemgr::ReturnCode::FAILED;

    auto transmitter = ADSWManager::Get() ? ADSWManager::Get()->GetTransmitter() : nullptr;
    const char* resp_topic = ADSWManager::Get() ?
        ADSWManager::Get()->GetTopic(MessageType::START_CONTAINER_RESPONSE) : nullptr;
    if (transmitter && resp_topic) {
        auto endpoint = transmitter->GetMessageEndpoint(resp_topic);
        if (endpoint) endpoint->Write(&resp_);
    }
}

} // namespace adsw_manager
} // namespace ros2
