#ifndef MOCK_LIFECYCLE_FACTORY_HPP_
#define MOCK_LIFECYCLE_FACTORY_HPP_

#include "manager/LifecycleFactory.hpp"
#include "state/StateMonitor.hpp"
#include "state/StateManager.hpp"
#include "service/ServiceRegistry.hpp"

namespace lifecycle_manager {
class MockLifecycleFactory : public LifecycleFactory {
public:
   // Only mock virtual methods


    MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
    MOCK_METHOD(std::shared_ptr<StateManager>, GetStateMgr, (), ());
    MOCK_METHOD(std::shared_ptr<StateMonitor>, GetMonitor, (), ());
    MOCK_METHOD(std::shared_ptr<PolicyManager>, GetPolicy, (), ());
    MOCK_METHOD(std::shared_ptr<MWServiceManager>, GetMWService, (), ());
    MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetRegistry, (), ());

};

inline MockLifecycleFactory* Mock_LifecycleFactory = nullptr;

LifecycleFactory::LifecycleFactory() {}
LifecycleFactory::~LifecycleFactory() {}

std::shared_ptr<LifecycleManager> LifecycleFactory::GetManager(){
    return Mock_LifecycleFactory->GetManager();
}

std::shared_ptr<DDSTransmitter> LifecycleFactory::GetTransmitter(){
    return Mock_LifecycleFactory->GetTransmitter();
}
std::shared_ptr<StateManager> LifecycleFactory::GetStateMgr(){
    return Mock_LifecycleFactory->GetStateMgr();
}
std::shared_ptr<StateMonitor> LifecycleFactory::GetMonitor(){
    return Mock_LifecycleFactory->GetMonitor();
}
std::shared_ptr<PolicyManager> LifecycleFactory::GetPolicy(){
    return Mock_LifecycleFactory->GetPolicy();
}
std::shared_ptr<MWServiceManager> LifecycleFactory::GetMWService(){
    return Mock_LifecycleFactory->GetMWService();
}
std::shared_ptr<ServiceRegistry> LifecycleFactory::GetRegistry(){
    return Mock_LifecycleFactory->GetRegistry();
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_FACTORY_HPP_
