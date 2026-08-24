
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
#include "lifecycle/NotifyContainerState.hpp"  // F008: aggregated container report

namespace lifecycle_manager {

class StateMonitor : public ManagerBase {
  public:
    StateMonitor(LifecycleManager& lifecycle);
    ~StateMonitor() override;

    bool Start();
    void Stop();
    
    void UpdateServicesState(lifecyclemgr::NotifyServiceState* update);
    void UpdateServicesState(lifecyclemgr::GetServiceStateResponse* response);

    void SetUpdateStateFlag(const std::shared_ptr<Service>& service);
    void ClearUpdateStateFlag(const std::shared_ptr<Service>& service);
    bool GetUpdateStateFlag(const std::shared_ptr<Service>& service);

    void ClearAllUpdatedFlag();

    std::set<std::shared_ptr<Service>> GetUpdateServiceList();

    void NotifyLifecycleDiagnosticInfo(bool all = false);

  private:
    void StartThread();
    void StopThread();
    void RunThread();
    void RequestGetState();
    void updateServices(std::vector<std::string>* slist, std::vector<lifecyclemgr::ServiceState> *statelist);
    bool getServiceState(const std::shared_ptr<Service>& service, lifecyclemgr::ServiceStatus& entry);

    // Polling interval (DESIGN-3): use named constants so the values are
    // visible and can be moved to config in the future.
    //
    // Latency: since updateServices() now publishes a diag report the moment a
    // service actually transitions (publish-on-change), the periodic report is a
    // reconciliation heartbeat rather than the primary path — so the poll can be
    // tighter (fallback for services that don't push NotifyServiceState) without
    // the monitor depending on it for responsiveness. 5s poll x 3 = 15s heartbeat.
    static constexpr int32_t kPollIntervalMs     = 5000;  // state-query period (non-push fallback)
    static constexpr int32_t kDiagReportInterval = 3;     // heartbeat diag every N polls

  private:
    std::set<std::shared_ptr<Service>> udpated_service_list_;

    std::thread processing_thread_;
    std::atomic<bool> stop_thread_{false};

    std::mutex lock_mutex_;
    // Serializes NotifyLifecycleDiagnosticInfo: it is now called from BOTH the
    // periodic RunThread AND the DDS receive thread (on-change publish), and it
    // reuses the diag_report_/container_report_ member buffers as the SendMessage
    // payload, so concurrent calls would otherwise race on those buffers.
    std::mutex publish_mutex_;

    lifecyclemgr::GetServiceStateRequest get_request_;
    lifecyclemgr::LifecycleDiagReport diag_report_;
    // F008: member (not local) — SendMessage queues the pointer and the publish
    // loop derefs it asynchronously, so the buffer must outlive the call.
    lifecyclemgr::NotifyContainerState container_report_;

    int32_t loop_count_ = 0;   // RunThread() 가 다시 0 을 넣지만 그 전에 읽힐 수 있다

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MONITOR_HPP_
