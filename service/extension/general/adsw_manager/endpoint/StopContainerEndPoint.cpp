#include "StopContainerEndPoint.hpp"

#include "lifecycle/StopContainerRequest.hpp"
#include "ADSWManager.hpp"
#include "control/ContainerController.hpp"
#include "control/ContainerServiceController.hpp"
#include "transmitter/DDSTransmitter.hpp"

#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {

StopContainerEndPoint::StopContainerEndPoint(MessageType msgtype, std::string topic)
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::STOP_CONTAINER_REQUEST) {
        type_support_ = std::make_shared<TypeSupport>(new StopContainerRequestPubSubType());
    } else if (msgtype == MessageType::STOP_CONTAINER_RESPONSE) {
        type_support_ = std::make_shared<TypeSupport>(new StopContainerResponsePubSubType());
    }
}

StopContainerEndPoint::~StopContainerEndPoint() {}

std::string StopContainerEndPoint::GetTopic() { return topic_; }
std::string StopContainerEndPoint::GetTypeName() { return type_support_->get_type_name(); }
std::shared_ptr<TypeSupport> StopContainerEndPoint::GetTypeSupport() { return type_support_; }

void StopContainerEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<StopContainerRequest>(req_) != true) return;

    StopContainerRequest* message = reinterpret_cast<StopContainerRequest*>(&req_);

    auto controller    = ADSWManager::Get() ? ADSWManager::Get()->GetServiceController() : nullptr;
    auto container_ctrl = controller ? controller->GetContainer() : nullptr;
    bool ok = (container_ctrl != nullptr);
    if (!ok) {
        LOG_ERROR("StopContainer: container controller unavailable");
    }

    for (const auto& name : message->container_list()) {
        if (!container_ctrl) { ok = false; break; }
        bool r = container_ctrl->StopContainer(name);
        LOG_DEBUG("StopContainer name=" << name << " result=" << (r ? "OK" : "FAIL"));
        if (!r) ok = false;
    }

    resp_.ret_code() = ok ? lifecyclemgr::ReturnCode::SUCCESS
                          : lifecyclemgr::ReturnCode::FAILED;

    auto transmitter = ADSWManager::Get() ? ADSWManager::Get()->GetTransmitter() : nullptr;
    const char* resp_topic = ADSWManager::Get() ?
        ADSWManager::Get()->GetTopic(MessageType::STOP_CONTAINER_RESPONSE) : nullptr;
    if (transmitter && resp_topic) {
        auto endpoint = transmitter->GetMessageEndpoint(resp_topic);
        if (endpoint) endpoint->Write(&resp_);
    }
}

} // namespace adsw_manager
} // namespace general
