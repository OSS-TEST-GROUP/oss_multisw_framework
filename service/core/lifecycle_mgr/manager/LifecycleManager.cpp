#include "LifecycleManager.hpp"

#include "service/ServiceRegistry.hpp"
#include "state/StateManager.hpp"
#include "state/StateMonitor.hpp"

#include "service/MWServiceManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "policy/PolicyManager.hpp"
#include "endpoint/EndPointFactory.hpp"
#include "logger/Log.hpp"

// F013: full IDL definitions required by the lambdas below
#include "lifecycle/NotifyContainerState.hpp"
#include "lifecycle/StartContainerRequest.hpp"
#include "lifecycle/StopContainerRequest.hpp"
#include "lifecycle/LifecycleCtrlRequest.hpp"
#include "lifecycle/SetRecoveryPolicyRequest.hpp"  // F014 follow-up
#include "include/TypeConvert.hpp"

namespace lifecycle_manager {

LifecycleManager::LifecycleManager() {}
LifecycleManager::~LifecycleManager() {}

bool LifecycleManager::Initialize() {
    LOG_DEBUG("LifecycleManager::Initialize" );

    SetState(ManagerState::MANAGER_INIT);

    service_registry_ = std::make_shared<ServiceRegistry>();
    state_manager_ = std::make_shared<StateManager>(*this);

    state_monitor_ = std::make_shared<StateMonitor>(*this);

    policy_manager_ = std::make_shared<PolicyManager>();

    mw_service_manager_ = std::make_shared<MWServiceManager>();
    mw_service_manager_->Initialize();
    is_k3s_mode_ = mw_service_manager_->IsK3sMode();
    LOG_DEBUG("LifecycleManager::Initialize k3s_mode = " << is_k3s_mode_);

    msg_transmitter_ = std::make_shared<DDSTransmitter>();
    msg_transmitter_->Initialize();

    endpoint_factory_ = new EndPointFactory();
    endpoint_factory_->Initialize();

    EndPointHandlers handlers;
    handlers.on_start = [this](lifecyclemgr::StartServiceResponse* msg) {
        if (state_manager_) state_manager_->OnStart(msg);
    };
    handlers.on_stop = [this](lifecyclemgr::StopServiceResponse* msg) {
        if (state_manager_) state_manager_->OnStop(msg);
    };
    handlers.on_set_mode = [this](lifecyclemgr::SetServiceModeRequest* msg) {
        if (state_manager_) state_manager_->SetServiceMode(msg);
    };
    handlers.on_get_state_resp = [this](lifecyclemgr::GetServiceStateResponse* msg) {
        if (state_monitor_) state_monitor_->UpdateServicesState(msg);
    };
    handlers.on_notify_state = [this](lifecyclemgr::NotifyServiceState* msg) {
        if (state_monitor_) state_monitor_->UpdateServicesState(msg);
    };
    handlers.on_update_list = [this](lifecyclemgr::UpdateServiceListRequest* msg) {
        if (service_registry_) service_registry_->UpdateServiceList(msg);
    };

    // F013: container actuator notifications from adsw_manager.
    // NotifyContainerState carries parallel arrays {name[i], state[i], exit[i]}.
    // We forward each to the registry so subsequent registry queries reflect
    // the latest known state.
    handlers.on_notify_container_state = [this](lifecyclemgr::NotifyContainerState* msg) {
        if (!service_registry_ || msg == nullptr) return;
        auto& names  = msg->container_list();
        auto& states = msg->container_state_list();
        auto& exits  = msg->exit_code_list();
        const size_t n = names.size();
        for (size_t i = 0; i < n; ++i) {
            ContainerState s = (i < states.size())
                ? TypeConvert::ToContainerState(states[i])
                : ContainerState::CS_UNKNOWN;
            int32_t ec = (i < exits.size()) ? exits[i] : 0;
            service_registry_->UpdateContainerState(names[i], s, ec);
        }
    };
    handlers.on_start_container_resp = [this](lifecyclemgr::StartContainerResponse* msg) {
        if (state_manager_) state_manager_->OnStartContainer(msg);
    };
    handlers.on_stop_container_resp = [this](lifecyclemgr::StopContainerResponse* msg) {
        if (state_manager_) state_manager_->OnStopContainer(msg);
    };

    // F013: ctrl/* gate from monitor/operator.  Each lambda forwards to a
    // StateManager handler that does (a) policy gating, (b) forwarding to
    // the actuator topic (adsw_manager), and (c) CtrlResponse publish.
    handlers.on_ctrl_start_service = [this](lifecyclemgr::CtrlStartServiceRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlStartService(msg);
    };
    handlers.on_ctrl_stop_service = [this](lifecyclemgr::CtrlStopServiceRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlStopService(msg);
    };
    handlers.on_ctrl_set_service_mode = [this](lifecyclemgr::CtrlSetServiceModeRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlSetServiceMode(msg);
    };
    handlers.on_ctrl_start_container = [this](lifecyclemgr::CtrlStartContainerRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlStartContainer(msg);
    };
    handlers.on_ctrl_stop_container = [this](lifecyclemgr::CtrlStopContainerRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlStopContainer(msg);
    };

    // F014 follow-up: recovery policy actuator + ctrl gate.
    handlers.on_set_recovery_policy_resp = [this](lifecyclemgr::SetRecoveryPolicyResponse* msg) {
        if (state_manager_) state_manager_->OnSetRecoveryPolicy(msg);
    };
    handlers.on_ctrl_set_recovery_policy = [this](lifecyclemgr::CtrlSetRecoveryPolicyRequest* msg) {
        if (state_manager_) state_manager_->HandleCtrlSetRecoveryPolicy(msg);
    };

    endpoint_factory_->CreateEndPoints(msg_transmitter_, handlers);

    return true;
}

bool LifecycleManager::Start() {
    LOG_DEBUG("LifecycleManager::Start" );
    SetState(ManagerState::MANAGER_STARTED);

    if (!state_manager_->Start()){
        LOG_ERROR("Failed to Start " );
        return false;
    }

    if (!policy_manager_->Start()){
        LOG_ERROR("Failed to Start " );
        return false;
    }   
    if (!mw_service_manager_->Start()){
        LOG_ERROR("Failed to Start " );
        return false;
    }       

    if (!state_monitor_->Start()){
        LOG_ERROR("Failed to Start " );
        return false;
    }   

    SetState(ManagerState::MANAGER_RUNNING);

    return true;

}

void LifecycleManager::Stop() {

    SetState(ManagerState::MANAGER_TERMINATE);

}

std::shared_ptr<DDSTransmitter> LifecycleManager::GetTransmitter(){
    return msg_transmitter_;
}

EndPointFactory* LifecycleManager::GetEndpointFactory(){
    return endpoint_factory_;
}

std::shared_ptr<StateManager> LifecycleManager::GetStateMgr(){
    return state_manager_;
}

std::shared_ptr<StateMonitor> LifecycleManager::GetMonitor(){
    return state_monitor_;
}

std::shared_ptr<PolicyManager> LifecycleManager::GetPolicy(){
    return policy_manager_;
}

std::shared_ptr<MWServiceManager> LifecycleManager::GetMWService(){
    return mw_service_manager_;
}

std::shared_ptr<ServiceRegistry> LifecycleManager::GetRegistry(){
    return service_registry_;
}    

std::shared_ptr<Service> LifecycleManager::GetService(std::string name){

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();

    if (registry){
        return registry->GetService(name);
    }

    return nullptr;
}

void LifecycleManager::SetState(ManagerState state){

    if (manager_state_ == ManagerState::MANAGER_RUNNING){

        if (state == ManagerState::MANAGER_TERMINATE){
            // State changed to TERMINATE --> Stop all sub component !!
            state_manager_->RequestStopAll();
        }
    }
    manager_state_ = state;
}

} // namespace lifecycle_manager
