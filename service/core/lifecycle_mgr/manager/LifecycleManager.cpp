#include "LifecycleManager.hpp"

#include "service/ServiceRegistry.hpp"
#include "state/StateManager.hpp"
#include "state/StateMonitor.hpp"

#include "service/MWServiceManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "policy/PolicyManager.hpp"
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {

LifecycleManager::LifecycleManager() {}
LifecycleManager::~LifecycleManager() {}

bool LifecycleManager::Initialize() {

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


std::shared_ptr<Service> LifecycleManager::GetService(std::string name){

    std::shared_ptr<ServiceRegistry> registry = GetRegistry();

    if (registry){
        return registry->GetService(name);
    }

    return nullptr;
}


} // namespace lifecycle_manager
