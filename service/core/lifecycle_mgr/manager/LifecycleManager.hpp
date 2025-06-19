#ifndef LIFECYCLE_LIFECYCLE_MANAGER_HPP_
#define LIFECYCLE_LIFECYCLE_MANAGER_HPP_

#include <memory>


namespace lifecycle_manager {

class ServiceRegistry;
class StateManager;
class StateMonitor;
class PolicyManager;
class MWServiceManager;
class DDSTransmitter;
class EndPointFactory;
class Service;

class LifecycleManager {
  public:
    LifecycleManager();
    virtual ~LifecycleManager();

    bool Initialize();

    bool Start();
    void Stop();


    std::shared_ptr<DDSTransmitter> GetTransmitter(){
      return msg_transmitter_;
    }

    EndPointFactory* GetEndpointFactory(){
      return endpoint_factory_;
    }

    std::shared_ptr<StateManager> GetState(){
      return state_manager_;
    }

    std::shared_ptr<StateMonitor> GetMonitor(){
      return state_monitor_;
    }
    std::shared_ptr<PolicyManager> GetPolicy(){
      return policy_manager_;
    }
    std::shared_ptr<MWServiceManager> GetMWService(){
      return mw_service_manager_;
    }    
    
    std::shared_ptr<ServiceRegistry> GetRegistry(){
      return service_registry_;
    }    

    std::shared_ptr<Service> GetService(std::string name);

  private:
    std::shared_ptr<ServiceRegistry> service_registry_;
    std::shared_ptr<StateManager> state_manager_;
    std::shared_ptr<StateMonitor> state_monitor_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<MWServiceManager> mw_service_manager_;
    std::shared_ptr<DDSTransmitter> msg_transmitter_;
    
    EndPointFactory* endpoint_factory_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_LIFECYCLE_MANAGER_HPP_

