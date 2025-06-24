
#ifndef LIFECYCLE_STATE_MONITOR_HPP_
#define LIFECYCLE_STATE_MONITOR_HPP_

#include <atomic>
#include <string>
#include <set>
#include <memory>
#include <thread>
#include <mutex>

#include "manager/ManagerBase.hpp"
#include "service/Service.hpp"

#include "lifecycle/GetServiceStateRequest.hpp"
#include "lifecycle/NotifyServiceState.hpp"
#include "lifecycle/LifecycleDiagReport.hpp"

namespace lifecycle_manager {

class StateMonitor : public ManagerBase {
  public:
    StateMonitor(LifecycleManager& lifecycle);
    virtual ~StateMonitor();

    bool Start();
    void Stop();
    
    void UpdateServicesState(::NotifyServiceState* update);
    void UpdateServicesState(::GetServiceStateResponse* response);

    void SetUpdateStateFlag(std::shared_ptr<Service> service);
    void ClearUpdateStateFlag(std::shared_ptr<Service> service);
    bool GetUpdateStateFlag(std::shared_ptr<Service> service);

    void ClearAllUpdatedFlag();

    std::set<std::shared_ptr<Service>> GetUpdateServiceList();

    void NotifyLifecycleDiagnosticInfo(bool all = false);

  private:
    void StartThread();
    void StopThread();
    void RunThread();
    void RequestGetState();
    void updateServices(std::vector<std::string>* slist, std::vector<::ServiceState> *statelist);
    bool getServiceState(std::shared_ptr<Service> service, ::ServiceStatus& entry);

  private:
    std::set<std::shared_ptr<Service>> udpated_service_list_;

    std::thread processing_thread_;
    std::atomic<bool> stop_thread_{false};

    std::mutex lock_mutex_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MONITOR_HPP_
