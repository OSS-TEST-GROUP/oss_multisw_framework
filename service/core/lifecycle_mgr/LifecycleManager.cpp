#include "LifecycleManager.hpp"

namespace lifecycle_manager {

LifecycleManager::LifecycleManager() {}
LifecycleManager::~LifecycleManager() {}

bool LifecycleManager::Initialize() {

    service_registry_ = std::make_shared<ServiceRegistry>();
    state_manager_ = std::make_shared<StateManager>();

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    state_monitor_ = std::make_shared<StateMonitor>();
     std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    policy_manager_ = std::make_shared<PolicyManager>();
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    mw_service_manager_ = std::make_shared<MWServiceManager>();
    mw_service_manager_->Initialize();
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    msg_transmitter_ = std::make_shared<DDSTransmitter>();
    msg_transmitter_->Initialize();

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    endpoint_factory_ = new EndPointFactory();
    endpoint_factory_->CreateEndPoints(msg_transmitter_);
    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;
}

bool LifecycleManager::Start() {

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

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

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    if (!mw_service_manager_->Start()){
        std::cerr << "Failed to initialize the MWServiceManager." << std::endl;
        return false;
    }       

    std::cout << __FILE__ << " :: " << __LINE__ << std::endl;

    return true;

}

void LifecycleManager::Stop() {

}

} // namespace lifecycle_manager
