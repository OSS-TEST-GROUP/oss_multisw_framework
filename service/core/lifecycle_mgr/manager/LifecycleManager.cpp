#include "LifecycleManager.hpp"

#include "service/ServiceRegistry.hpp"
#include "state/StateManager.hpp"
#include "state/StateMonitor.hpp"

#include "service/MWServiceManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "policy/PolicyManager.hpp"
#include "endpoint/EndPointFactory.hpp"
#include "logger/Log.hpp"

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
