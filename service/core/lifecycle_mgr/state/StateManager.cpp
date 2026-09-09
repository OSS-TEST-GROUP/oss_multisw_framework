#include <iostream>
#include <map>

#include "StateManager.hpp"
#include "include/MwType.hpp"

#include "manager/ManagerBase.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "include/TypeConvert.hpp"

#include "service/Service.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

StateManager::StateManager(LifecycleManager& lifecycle)
    : ManagerBase(lifecycle){
}


bool StateManager::Start() {

    return true;
}

void StateManager::Stop() {

    
}

bool StateManager::StartService(const std::shared_ptr<Service>& service) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    start_request_.service_list().clear();
     
    if (transmitter != nullptr)
    {
        start_request_.mw_type() = TypeConvert::ToMwType(service->GetMwType());
        start_request_.service_list().push_back(service->GetName());
        start_request_.is_group() = false;
        ++start_in_flight_;
        return transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
    }

    return false;
}

void StateManager::StopService(const std::shared_ptr<Service>& service) {

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    stop_request_.service_list().clear();

    if (transmitter != nullptr)
    {
        stop_request_.mw_type() = TypeConvert::ToMwType(service->GetMwType());
        stop_request_.service_list().push_back(service->GetName());
        stop_request_.is_group() = false;
        ++stop_in_flight_;
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
    }
}

bool StateManager::SetServiceMode(lifecyclemgr::SetServiceModeRequest* message){

    std::vector<std::string> *slist = &message->service_list();
    
    ServiceMode service_mode = TypeConvert::ToServiceMode(message->service_mode());

    for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
        std::shared_ptr<Service> service = GetService(*svc);

        if (service != nullptr){
            service->SetServiceMode(service_mode);
        }
    }
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter != nullptr)
    {
        set_response_.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
        transmitter->SendMessage(MessageType::SET_SERVICE_MODE_RESPONSE, reinterpret_cast<void*>(&set_response_));
    }
    return true;
}

void StateManager::ProcessServiceHandling(const std::set<std::shared_ptr<Service>>& service_list) {

    /* udpated service list passed */

    std::shared_ptr<PolicyManager> policy_manager = GetPolicy();

    // MwType별로 START/STOP 목록 분리
    std::map<MwType, std::vector<std::string>> start_map;
    std::map<MwType, std::vector<std::string>> stop_map;

    for( auto it_service = service_list.begin(); it_service != service_list.end(); ++it_service ){

        const std::shared_ptr<Service>& service = *it_service;
        ServiceControlAction action = policy_manager->Evaluate(service);

        switch (action)
        {
        case ServiceControlAction::START:
            start_map[service->GetMwType()].push_back(service->GetName());
            break;

        case ServiceControlAction::STOP:
            stop_map[service->GetMwType()].push_back(service->GetName());
            break;

        default:
            break;
        }
    }

    if (IsK3sMode()) {
        // K3S 모드: 향후 K3SInterface 호출로 교체
        LOG_DEBUG("StateManager::ProcessServiceHandling - K3S mode (not implemented yet)");
        return;
    }

    // Non-K3S 모드: adsw_manager에 DDS 명령 전송
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr) {
        return;
    }

    for (auto& entry : start_map){
        start_request_.service_list() = entry.second;
        start_request_.mw_type() = TypeConvert::ToMwType(entry.first);
        start_request_.is_group() = false;
        LOG_DEBUG("StateManager: START " << entry.second.size() << " service(s) for MwType "
                  << static_cast<int>(entry.first));
        ++start_in_flight_;
        transmitter->SendMessage(MessageType::START_SERVICE_REQUEST, reinterpret_cast<void*>(&start_request_));
    }

    for (auto& entry : stop_map){
        stop_request_.service_list() = entry.second;
        stop_request_.mw_type() = TypeConvert::ToMwType(entry.first);
        stop_request_.is_group() = false;
        LOG_DEBUG("StateManager: STOP " << entry.second.size() << " service(s) for MwType "
                  << static_cast<int>(entry.first));
        ++stop_in_flight_;
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
    }
}

void StateManager::RequestStopAll(){

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();

    if (transmitter != nullptr)
    {
        std::shared_ptr<ServiceRegistry> registry = GetRegistry();

        if (registry){
            
            std::vector<std::string> groups = registry->GetGroupList();

            for (auto& group : groups){
                std::vector<std::string> list = registry->GetServicesNameByGroup(group);    
                stop_request_.service_list().clear();
                for( auto svc = list.begin(); svc != list.end(); ++svc ){
                    stop_request_.service_list().push_back(*svc);
                }
                stop_request_.is_group() = false;

                ++stop_in_flight_;
                transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST, reinterpret_cast<void*>(&stop_request_));
            }
        }
    }
}

void StateManager::OnStart(lifecyclemgr::StartServiceResponse* message){
    if (!message) {
        return;
    }
    // Phantom-response guard. A zombie DDS participant on the same domain can
    // replay a stale StartServiceResponse(ret_code=FAILED) on
    // mmf/lifecyclemgr/StartServiceResponse even when we haven't dispatched a
    // request. Before this guard, those phantoms caused a steady stream of
    // "Service Start Failure 1" and forwarded a FAILED ctrl response to
    // lfc_monitor, making the operator think a healthy service was broken.
    // We only accept a response when at least one of our own StartService
    // requests is outstanding.
    int expected = start_in_flight_.load(std::memory_order_acquire);
    while (expected > 0) {
        if (start_in_flight_.compare_exchange_weak(expected, expected - 1,
                                                    std::memory_order_acq_rel)) {
            break;
        }
    }
    if (expected <= 0) {
        LOG_DEBUG("OnStart: dropping spurious StartServiceResponse (no in-flight "
                  "StartServiceRequest); ret_code="
                  << static_cast<int32_t>(message->ret_code()));
        return;
    }

    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Service Start Failure " << static_cast<int32_t>(message->ret_code()));
    }
    // F014 follow-up: keep ctrl/* response semantics symmetric with the
    // container path (OnStartContainer).  Every StartServiceResponse the
    // actuator emits is relayed to ctrl/StartServiceResponse so the
    // monitor sees the real end-to-end verdict, not just a "forwarded"
    // ack.  Non-ctrl-originated starts (e.g. recovery policy retries)
    // also relay; that's harmless — the monitor's events ring shows
    // operator visibility into the same outcome.
    PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE, message->ret_code());
}

void StateManager::OnStop(lifecyclemgr::StopServiceResponse* message){
    if (!message) {
        return;
    }
    // Same phantom guard as OnStart — see commentary there.
    int expected = stop_in_flight_.load(std::memory_order_acquire);
    while (expected > 0) {
        if (stop_in_flight_.compare_exchange_weak(expected, expected - 1,
                                                   std::memory_order_acq_rel)) {
            break;
        }
    }
    if (expected <= 0) {
        LOG_DEBUG("OnStop: dropping spurious StopServiceResponse (no in-flight "
                  "StopServiceRequest); ret_code="
                  << static_cast<int32_t>(message->ret_code()));
        return;
    }

    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Service Stop Failure " << static_cast<int32_t>(message->ret_code()));
    }
    PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE, message->ret_code());
}

// ── F013: actuator response relays ─────────────────────────────────────

void StateManager::OnStartContainer(lifecyclemgr::StartContainerResponse* message){
    if (!message) {
        return;
    }
    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Container Start Failure " << static_cast<int32_t>(message->ret_code()));
    }
    // Relay verdict to ctrl/* response so the monitor sees the final outcome
    // of the CtrlStartContainerRequest it issued.
    PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE, message->ret_code());
}

void StateManager::OnStopContainer(lifecyclemgr::StopContainerResponse* message){
    if (!message) {
        return;
    }
    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("Container Stop Failure " << static_cast<int32_t>(message->ret_code()));
    }
    PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE, message->ret_code());
}

// ── F013: ctrl/* gate handlers ────────────────────────────────────────
//
// Gating policy (option (b) chosen at design time):
//   - Empty request list   → FAILED (nothing to act on, surface immediately).
//   - Unknown service/container name → FAILED.
//   - Service mode mismatch (e.g. control while in OTA_MODE) → FAILED.
//   - Otherwise forward to the actuator topic (adsw_manager) and let the
//     OnStart/OnStop/OnStart/StopContainer relays publish the
//     ctrl/*Response with the real actuator verdict.  No interim SUCCESS
//     ack — every ctrl/* command shares one shape:
//       "lifecycle_mgr's ctrl/*Response == actuator's success/fail",
//     mirroring how StartContainer already worked. The operator never
//     sees an early SUCCESS that's contradicted by a later failure on a
//     separate channel.
//
// Every rejection below MUST log.  `CtrlResponse` carries a single field
// (`ReturnCode ret_code`) — no target name, no request id — so a FAILED
// that reaches lfc_monitor is anonymous by construction.  The core log is
// the only place the cause survives.  Measured 2026-09-07: a steady
// ctrl/StartServiceResponse FAILED stream on board 81 was untraceable
// because the empty-list and no-transmitter branches published FAILED
// without a single log line, while the paths that *did* log showed 0 hits.

void StateManager::HandleCtrlStartService(lifecyclemgr::CtrlStartServiceRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        LOG_ERROR("CtrlStartService: empty service_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    // Group services by MwType for the actuator topic shape.
    std::map<MwType, std::vector<std::string>> by_os;
    for (const auto& name : names){
        std::shared_ptr<Service> service = GetService(name);
        if (service == nullptr){
            LOG_ERROR("CtrlStartService: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        if (service->GetServiceMode() == ServiceMode::OTA_MODE){
            LOG_ERROR("CtrlStartService: service '" << name << "' is in OTA_MODE — rejected");
            PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        by_os[service->GetMwType()].push_back(name);
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlStartService: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_START_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    for (auto& entry : by_os){
        start_request_.service_list() = entry.second;
        start_request_.mw_type() = TypeConvert::ToMwType(entry.first);
        start_request_.is_group() = false;
        ++start_in_flight_;
        transmitter->SendMessage(MessageType::START_SERVICE_REQUEST,
                                  reinterpret_cast<void*>(&start_request_));
    }
    // No interim ctrl/StartServiceResponse here — OnStart() relays the
    // real actuator verdict once StartServiceResponse comes back.
}

void StateManager::HandleCtrlStopService(lifecyclemgr::CtrlStopServiceRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        LOG_ERROR("CtrlStopService: empty service_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::map<MwType, std::vector<std::string>> by_os;
    for (const auto& name : names){
        std::shared_ptr<Service> service = GetService(name);
        if (service == nullptr){
            LOG_ERROR("CtrlStopService: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
        by_os[service->GetMwType()].push_back(name);
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlStopService: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_STOP_SERVICE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    for (auto& entry : by_os){
        stop_request_.service_list() = entry.second;
        stop_request_.mw_type() = TypeConvert::ToMwType(entry.first);
        stop_request_.is_group() = false;
        ++stop_in_flight_;
        transmitter->SendMessage(MessageType::STOP_SERVICE_REQUEST,
                                  reinterpret_cast<void*>(&stop_request_));
    }
    // No interim ctrl/StopServiceResponse — OnStop() relays the real
    // actuator verdict once StopServiceResponse comes back.
}

void StateManager::HandleCtrlSetServiceMode(lifecyclemgr::CtrlSetServiceModeRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        LOG_ERROR("CtrlSetServiceMode: empty service_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }
    for (const auto& name : names){
        if (GetService(name) == nullptr){
            LOG_ERROR("CtrlSetServiceMode: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlSetServiceMode: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    // Forward through the existing SetServiceMode actuator path —
    // adsw_manager will pause / resume the matching ROS2 services.
    set_mode_request_.service_list() = names;
    set_mode_request_.service_mode() = message->service_mode();
    transmitter->SendMessage(MessageType::SET_SERVICE_MODE_REQUST,
                              reinterpret_cast<void*>(&set_mode_request_));

    // Mirror the mode update into the local registry so subsequent
    // CtrlStart/Stop gating decisions see the new mode immediately —
    // adsw_manager has no callback path for SetServiceMode.
    ServiceMode mode = TypeConvert::ToServiceMode(message->service_mode());
    for (const auto& name : names){
        auto svc = GetService(name);
        if (svc) {
            svc->SetServiceMode(mode);
        }
    }

    PublishCtrlResponse(MessageType::CTRL_SET_SERVICE_MODE_RESPONSE,
                        lifecyclemgr::ReturnCode::SUCCESS);
}

void StateManager::HandleCtrlStartContainer(lifecyclemgr::CtrlStartContainerRequest* message){
    if (!message){ return; }

    auto& names = message->container_list();
    if (names.empty()){
        LOG_ERROR("CtrlStartContainer: empty container_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();
    if (registry){
        for (const auto& name : names){
            if (registry->GetContainer(name) == nullptr){
                LOG_ERROR("CtrlStartContainer: unknown container '" << name << "'");
                PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                                    lifecyclemgr::ReturnCode::FAILED);
                return;
            }
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlStartContainer: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_START_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    start_container_request_.container_list() = names;
    transmitter->SendMessage(MessageType::START_CONTAINER_REQUEST,
                              reinterpret_cast<void*>(&start_container_request_));
    // Note: the *final* SUCCESS/FAIL CtrlResponse is published by
    // OnStartContainer() when adsw_manager's StartContainerResponse arrives.
    // We don't publish an interim SUCCESS here to avoid double-firing the
    // monitor's pending state.
}

void StateManager::HandleCtrlStopContainer(lifecyclemgr::CtrlStopContainerRequest* message){
    if (!message){ return; }

    auto& names = message->container_list();
    if (names.empty()){
        LOG_ERROR("CtrlStopContainer: empty container_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();
    if (registry){
        for (const auto& name : names){
            if (registry->GetContainer(name) == nullptr){
                LOG_ERROR("CtrlStopContainer: unknown container '" << name << "'");
                PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                                    lifecyclemgr::ReturnCode::FAILED);
                return;
            }
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlStopContainer: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_STOP_CONTAINER_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    stop_container_request_.container_list() = names;
    transmitter->SendMessage(MessageType::STOP_CONTAINER_REQUEST,
                              reinterpret_cast<void*>(&stop_container_request_));
}

bool StateManager::PublishCtrlResponse(MessageType response_topic,
                                        lifecyclemgr::ReturnCode code){
    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr) {
        return false;
    }
    ctrl_response_.ret_code() = code;
    return transmitter->SendMessage(response_topic,
                                     reinterpret_cast<void*>(&ctrl_response_));
}

// ── F014 follow-up: recovery policy mutation ──────────────────────────
//
// monitor → ctrl/SetRecoveryPolicy → lifecycle_mgr policy gate → forward
// to SetRecoveryPolicyRequest (actuator topic on adsw_manager) → adsw
// mutates ServiceRegistry + persists yaml → adsw publishes
// SetRecoveryPolicyResponse → lifecycle_mgr OnSetRecoveryPolicy() relays
// to ctrl/SetRecoveryPolicyResponse for monitor.  Same end-to-end shape
// as Start/StopContainer.

void StateManager::HandleCtrlSetRecoveryPolicy(
        lifecyclemgr::CtrlSetRecoveryPolicyRequest* message){
    if (!message){ return; }

    auto& names = message->service_list();
    if (names.empty()){
        LOG_ERROR("CtrlSetRecoveryPolicy: empty service_list — rejected");
        PublishCtrlResponse(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }
    for (const auto& name : names){
        if (GetService(name) == nullptr){
            LOG_ERROR("CtrlSetRecoveryPolicy: unknown service '" << name << "'");
            PublishCtrlResponse(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE,
                                lifecyclemgr::ReturnCode::FAILED);
            return;
        }
    }

    std::shared_ptr<DDSTransmitter> transmitter = GetTransmitter();
    if (transmitter == nullptr){
        LOG_ERROR("CtrlSetRecoveryPolicy: no DDS transmitter — rejected");
        PublishCtrlResponse(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE,
                            lifecyclemgr::ReturnCode::FAILED);
        return;
    }

    set_recovery_request_.service_list() = names;
    set_recovery_request_.recovery_policy() = message->recovery_policy();
    transmitter->SendMessage(MessageType::SET_RECOVERY_POLICY_REQUEST,
                              reinterpret_cast<void*>(&set_recovery_request_));

    // Mirror into local registry immediately so subsequent gating
    // decisions see the new policy without waiting for the round trip.
    RecoveryPolicy policy =
        TypeConvert::ToRecoveryPolicy(message->recovery_policy());
    for (const auto& name : names){
        auto svc = GetService(name);
        if (svc) {
            svc->SetRecoveryPolicy(policy);
        }
    }
    // Final ctrl/* response is published by OnSetRecoveryPolicy when the
    // SetRecoveryPolicyResponse arrives — same pattern as Start/StopContainer.
}

void StateManager::OnSetRecoveryPolicy(
        lifecyclemgr::SetRecoveryPolicyResponse* message){
    if (!message) {
        return;
    }
    if (message->ret_code() != lifecyclemgr::ReturnCode::SUCCESS){
        LOG_ERROR("SetRecoveryPolicy Failure "
                  << static_cast<int32_t>(message->ret_code()));
    }
    PublishCtrlResponse(MessageType::CTRL_SET_RECOVERY_POLICY_RESPONSE,
                        message->ret_code());
}


} // namespace lifecycle_manager
