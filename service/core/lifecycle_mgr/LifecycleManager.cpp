#include "LifecycleManager.hpp"

namespace lifecycle_manager {

LifecycleManager::LifecycleManager() {}
LifecycleManager::~LifecycleManager() {}

bool LifecycleManager::Initialize() {

    service_registry_ = std::make_shared<ServiceRegistry>();
    state_manager_ = std::make_shared<StateManager>();


    state_monitor_ = std::make_shared<StateMonitor>();
 
    policy_manager_ = std::make_shared<PolicyManager>();

    mw_service_manager_ = std::make_shared<MWServiceManager>();
    mw_service_manager_->Initialize();

    msg_transmitter_ = std::make_shared<DDSTransmitter>();
    msg_transmitter_->Initialize();

    return true;
}

bool LifecycleManager::Start() {

    if (!state_manager_->Start()){
        std::cerr << "Failed to initialize the StateManager." << std::endl;
        return false;
    }

    if (!state_monitor_->Start()){
        std::cerr << "Failed to initialize the StateMonitor." << std::endl;
        return false;
    }   

    if (!policy_manager_->Start()){
        std::cerr << "Failed to initialize the PolicyManager." << std::endl;
        return false;
    }   
    
    if (!mw_service_manager_->Start()){
        std::cerr << "Failed to initialize the MWServiceManager." << std::endl;
        return false;
    }       
    return true;

}
void LifecycleManager::Stop() {

}

} // namespace lifecycle_manager
