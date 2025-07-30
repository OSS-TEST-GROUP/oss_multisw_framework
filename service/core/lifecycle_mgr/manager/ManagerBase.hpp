#ifndef LIFECYCLE_MANAGER_BASE_HPP_
#define LIFECYCLE_MANAGER_BASE_HPP_

#include "manager/LifecycleManager.hpp"
#include "include/MessageType.hpp"

namespace lifecycle_manager {

class LifecycleManager;
class DDSTransmitter;
class Service;

class ManagerBase {
  public:
    ManagerBase(LifecycleManager& lifecycle);
    virtual ~ManagerBase();

    std::shared_ptr<DDSTransmitter> GetTransmitter();
    std::shared_ptr<Service> GetService(std::string name);
    std::shared_ptr<ServiceRegistry> GetRegistry();
    std::shared_ptr<PolicyManager> GetPolicy();
    std::shared_ptr<StateManager> GetStateManager();
    
    const char* GetTopic(MessageType mtype);
    
  private:
    LifecycleManager& lifecycle_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MANAGER_BASE_HPP_
