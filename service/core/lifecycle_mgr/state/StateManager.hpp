#ifndef LIFECYCLE_STATE_MANAGER_HPP_
#define LIFECYCLE_STATE_MANAGER_HPP_

#include <atomic>
#include <memory>
#include <vector>

#include <thread>

#include "manager/ManagerBase.hpp"

#include "policy/PolicyManager.hpp"
#include "include/ServiceMode.hpp"

namespace lifecycle_manager {

class StateManager : public ManagerBase {
  public:
    StateManager(LifecycleManager& lifecycle);
    virtual ~StateManager();

    bool Start();
    void Stop();

    bool StartService(std::string name);
    void StopService(std::string name);

    bool SetServiceMode(std::string service, ServiceMode mode);
    void ProcessServiceHandling(std::vector<std::string> service_list);

  private:
    void StartThread();
    void StopThread();
    void RunThread();
    void RequestGetState();
    void OnGetServiceResponse(void* data);

  private:
    std::shared_ptr<PolicyManager> policy_manager_;

    std::thread processing_thread_;
    std::atomic<bool> stop_thread_{false};

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MANAGER_HPP_
