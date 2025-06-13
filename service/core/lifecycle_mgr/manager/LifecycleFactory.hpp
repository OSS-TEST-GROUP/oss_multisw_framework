#ifndef LIFECYCLE_LIFECYCLE_FACTORY_HPP_
#define LIFECYCLE_LIFECYCLE_FACTORY_HPP_

#include "manager/LifecycleManager.hpp"

namespace lifecycle_manager {


class LifecycleFactory {
  public:
    LifecycleFactory();
    virtual ~LifecycleFactory();

    static LifecycleFactory* GetInstance();

    std::shared_ptr<LifecycleManager> GetManager();

    std::shared_ptr<DDSTransmitter> GetTransmitter();
    std::shared_ptr<StateManager> GetState();
    std::shared_ptr<StateMonitor> GetMonitor();
    std::shared_ptr<PolicyManager> GetPolicy();
    std::shared_ptr<MWServiceManager> GetMWService();    
    std::shared_ptr<ServiceRegistry> GetRegistry();

  private :
    static std::shared_ptr<LifecycleFactory> getShared();

  private:
    std::shared_ptr<LifecycleManager> lifecycle_manager_;
};


} // namespace lifecycle_manager

#endif // LIFECYCLE_LIFECYCLE_FACTORY_HPP_
