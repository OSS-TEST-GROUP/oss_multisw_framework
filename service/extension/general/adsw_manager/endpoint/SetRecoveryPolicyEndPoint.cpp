#include "SetRecoveryPolicyEndPoint.hpp"

#include "lifecycle/SetRecoveryPolicyRequest.hpp"
#include "ADSWManager.hpp"
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/TypeConvert.hpp"
#include "transmitter/DDSTransmitter.hpp"

#include "logger/Log.hpp"

namespace general {
namespace adsw_manager {

SetRecoveryPolicyEndPoint::SetRecoveryPolicyEndPoint(MessageType msgtype, std::string topic)
    : DDSMessageEndpoint() {
    topic_ = topic;
    if (msgtype == MessageType::SET_RECOVERY_POLICY_REQUEST) {
        type_support_ = std::make_shared<TypeSupport>(new SetRecoveryPolicyRequestPubSubType());
    } else if (msgtype == MessageType::SET_RECOVERY_POLICY_RESPONSE) {
        type_support_ = std::make_shared<TypeSupport>(new SetRecoveryPolicyResponsePubSubType());
    }
}

SetRecoveryPolicyEndPoint::~SetRecoveryPolicyEndPoint() {}

std::string SetRecoveryPolicyEndPoint::GetTopic() { return topic_; }
std::string SetRecoveryPolicyEndPoint::GetTypeName() { return type_support_->get_type_name(); }
std::shared_ptr<TypeSupport> SetRecoveryPolicyEndPoint::GetTypeSupport() { return type_support_; }

void SetRecoveryPolicyEndPoint::OnDataAvailable() {
    LOG_DEBUG("Message Delivery for " << GetTopic());

    if (Read<SetRecoveryPolicyRequest>(req_) != true) return;

    SetRecoveryPolicyRequest* message = reinterpret_cast<SetRecoveryPolicyRequest*>(&req_);

    auto registry = ADSWManager::Get() ? ADSWManager::Get()->GetServiceRegistry() : nullptr;
    bool ok = (registry != nullptr);

    if (ok) {
        RecoveryPolicy policy = TypeConvert::ToRecoveryPolicy(message->recovery_policy());
        for (const auto& name : message->service_list()) {
            auto svc = registry->GetService(name);
            if (!svc) {
                LOG_ERROR("SetRecoveryPolicy: unknown service '" << name << "'");
                ok = false;
                continue;
            }
            svc->SetRecoveryPolicy(policy);
            LOG_DEBUG("SetRecoveryPolicy: " << name << " → "
                      << static_cast<int>(policy));
        }
        // Persist into saved-service-list.yaml so the change survives a
        // process restart.  Best-effort — log on failure but still
        // surface the in-memory mutation as SUCCESS to keep the gate
        // response useful; operator can detect persistence loss later.
        if (ok && !registry->Save()) {
            LOG_ERROR("SetRecoveryPolicy: Save() failed — change held in memory only");
        }
    } else {
        LOG_ERROR("SetRecoveryPolicy: registry unavailable");
    }

    resp_.ret_code() = ok ? lifecyclemgr::ReturnCode::SUCCESS
                          : lifecyclemgr::ReturnCode::FAILED;

    auto transmitter = ADSWManager::Get() ? ADSWManager::Get()->GetTransmitter() : nullptr;
    const char* resp_topic = ADSWManager::Get() ?
        ADSWManager::Get()->GetTopic(MessageType::SET_RECOVERY_POLICY_RESPONSE) : nullptr;
    if (transmitter && resp_topic) {
        auto endpoint = transmitter->GetMessageEndpoint(resp_topic);
        if (endpoint) endpoint->Write(&resp_);
    }
}

} // namespace adsw_manager
} // namespace general
