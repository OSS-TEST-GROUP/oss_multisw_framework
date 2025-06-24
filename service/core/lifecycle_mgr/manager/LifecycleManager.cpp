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

    service_registry_ = std::make_shared<ServiceRegistry>();
    state_manager_ = std::make_shared<StateManager>(*this);

    state_monitor_ = std::make_shared<StateMonitor>(*this);

    policy_manager_ = std::make_shared<PolicyManager>();

    mw_service_manager_ = std::make_shared<MWServiceManager>();
    mw_service_manager_->Initialize();

    msg_transmitter_ = std::make_shared<DDSTransmitter>();
    msg_transmitter_->Initialize();

    endpoint_factory_ = new EndPointFactory();
    endpoint_factory_->Initialize();
    endpoint_factory_->CreateEndPoints(msg_transmitter_);

    return true;
}

bool LifecycleManager::Start() {
    LOG_DEBUG("LifecycleManager::Start" );

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

    return true;

}

void LifecycleManager::Stop() {

}



    std::shared_ptr<DDSTransmitter> LifecycleManager::GetTransmitter(){
      return msg_transmitter_;
    }

    EndPointFactory* LifecycleManager::GetEndpointFactory(){
      return endpoint_factory_;
    }

    std::shared_ptr<StateManager> LifecycleManager::GetState(){
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


} // namespace lifecycle_manager
