#ifndef LIFECYCLE_STATE_MANAGER_HPP_
#define LIFECYCLE_STATE_MANAGER_HPP_

#include <memory>
#include <vector>

#include "policy/PolicyManager.hpp"
#include "include/ServiceMode.hpp"

namespace lifecycle_manager {

class StateManager {
  public:
    StateManager();
    virtual ~StateManager();

    bool Start();
    void Stop();

    bool StartService(std::string name);
    void StopService(std::string name);

    bool SetServiceMode(std::string service, ServiceMode mode);
    void ProcessServiceHandling(std::vector<std::string> service_list);

  private:
    std::shared_ptr<PolicyManager> policy_manager_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MANAGER_HPP_
