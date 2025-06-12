#ifndef LIFECYCLE_LIFECYCLE_MANAGER_HPP_
#define LIFECYCLE_LIFECYCLE_MANAGER_HPP_

#include <memory>

#include "service/ServiceRegistry.hpp"
#include "state/StateManager.hpp"
#include "state/StateMonitor.hpp"

#include "service/MWServiceManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "policy/PolicyManager.hpp"
#include "endpoint/EndPointFactory.hpp"

namespace lifecycle_manager {

class LifecycleManager {
  public:
    LifecycleManager();
    virtual ~LifecycleManager();

    bool Initialize();

    bool Start();
    void Stop();

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

