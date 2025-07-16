#ifndef LIFECYCLE_STATE_MANAGER_HPP_
#define LIFECYCLE_STATE_MANAGER_HPP_

#include <memory>
#include <vector>
#include <set>

#include "manager/ManagerBase.hpp"

#include "policy/PolicyManager.hpp"
#include "include/ServiceMode.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"

namespace lifecycle_manager {

class StateManager : public ManagerBase {
  public:
    StateManager(LifecycleManager& lifecycle);
    virtual ~StateManager();

    bool Start();
    void Stop();

    bool StartService(std::shared_ptr<Service> service);
    void StopService(std::shared_ptr<Service> service);

    bool SetServiceMode(lifecyclemgr::SetServiceModeRequest* message);
    void ProcessServiceHandling(std::set<std::shared_ptr<Service>> service_list);

    void OnStart(lifecyclemgr::StartServiceResponse* message);
    void OnStop(lifecyclemgr::StopServiceResponse* message);
    
  private:

  private:
    std::shared_ptr<PolicyManager> policy_manager_;


};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MANAGER_HPP_
