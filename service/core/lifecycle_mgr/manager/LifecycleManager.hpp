#ifndef LIFECYCLE_LIFECYCLE_MANAGER_HPP_
#define LIFECYCLE_LIFECYCLE_MANAGER_HPP_

#include <memory>

#include "include/ManagerState.hpp"

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
    virtual ~LifecycleManager() = default;
    bool Initialize();

    bool Start();
    void Stop();


    std::shared_ptr<DDSTransmitter> GetTransmitter();

    EndPointFactory* GetEndpointFactory();

    std::shared_ptr<StateManager> GetStateMgr();

    std::shared_ptr<StateMonitor> GetMonitor();
    std::shared_ptr<PolicyManager> GetPolicy();
    std::shared_ptr<MWServiceManager> GetMWService();
    std::shared_ptr<ServiceRegistry> GetRegistry();

    std::shared_ptr<Service> GetService(std::string name);

    ManagerState GetState() { return manager_state_; }
    void SetState(ManagerState state);

    bool IsK3sMode() const { return is_k3s_mode_; }

    // Unit test support: allow test subclasses to access dependency injection
  protected:
    std::shared_ptr<ServiceRegistry> service_registry_;
    std::shared_ptr<StateManager> state_manager_;
    std::shared_ptr<StateMonitor> state_monitor_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<MWServiceManager> mw_service_manager_;
    std::shared_ptr<DDSTransmitter> msg_transmitter_;
    // 생성자에서 초기화되지 않아 GetEndPointFactory() 가 Initialize() 전에
    // 호출되면 쓰레기 포인터를 반환했다 (cppcheck uninitMemberVar).
    EndPointFactory* endpoint_factory_ = nullptr;

    // 미초기화 enum 을 GetState() 가 그대로 반환하고 비교문에서도 읽혔다.
    ManagerState     manager_state_ = ManagerState::MANAGER_INIT;
    bool             is_k3s_mode_{false};
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_LIFECYCLE_MANAGER_HPP_

