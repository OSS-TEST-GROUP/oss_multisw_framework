
#ifndef LIFECYCLE_STATE_MONITOR_HPP_
#define LIFECYCLE_STATE_MONITOR_HPP_

#include <string>
#include <set>
#include <memory>

#include "service/Service.hpp"

namespace lifecycle_manager {

class StateMonitor {
  public:
    StateMonitor();
    virtual ~StateMonitor();

    void UpdateServicesState(std::string udpate_msg);

    void SetUpdateState(std::string service);
    void ClearUpdateState(std::string service);
    bool GetUdpateStateFlag(std::string service);

    void ClearAllUpdatedFlag();

    std::set<std::shared_ptr<Service>> GetUpdateServiceList();

    void NotifyLifecycleDiagnosticInfo();

  private:
    std::set<std::shared_ptr<Service>> udpated_service_list_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MONITOR_HPP_
