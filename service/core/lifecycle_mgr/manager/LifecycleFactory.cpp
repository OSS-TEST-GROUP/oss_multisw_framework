#include "LifecycleFactory.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

LifecycleFactory::LifecycleFactory() :lifecycle_manager_(nullptr)
{
    LOG_INFO("Create LifecycleFactory");
}

LifecycleFactory::~LifecycleFactory() {}

LifecycleFactory* LifecycleFactory::GetInstance(){
   return getShared().get();
}

std::shared_ptr<LifecycleFactory> LifecycleFactory::getShared()
{
    // Note we need a custom deleter, since the destructor is protected.
    static std::shared_ptr<LifecycleFactory> instance(
        new LifecycleFactory(),
        [](LifecycleFactory* p)
        {
            delete p;
        });
    return instance;
}

std::shared_ptr<LifecycleManager> LifecycleFactory::GetManager(){
    
    if (lifecycle_manager_ == nullptr){
        lifecycle_manager_ = std::make_shared<LifecycleManager>();
    }
    return lifecycle_manager_;

}

std::shared_ptr<DDSTransmitter> LifecycleFactory::GetTransmitter(){
    return GetManager()->GetTransmitter();
}

std::shared_ptr<StateManager> LifecycleFactory::GetState(){
    return GetManager()->GetState();
}

std::shared_ptr<StateMonitor> LifecycleFactory::GetMonitor(){
    return GetManager()->GetMonitor();
}

std::shared_ptr<PolicyManager> LifecycleFactory::GetPolicy(){
    return GetManager()->GetPolicy();
}
std::shared_ptr<MWServiceManager> LifecycleFactory::GetMWService(){
    return GetManager()->GetMWService();
}  

std::shared_ptr<ServiceRegistry> LifecycleFactory::GetRegistry(){
    return GetManager()->GetRegistry();
}


} // namespace lifecycle_manager
