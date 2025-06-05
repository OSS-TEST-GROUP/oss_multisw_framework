#ifndef LIFECYCLE_SERVICE_HPP_
#define LIFECYCLE_SERVICE_HPP_

#include <string>

#include "include/ServiceState.hpp"
#include "include/ServiceType.hpp"
#include "include/ServiceMode.hpp"
#include "include/RecoveryPolicy.hpp"

namespace lifecycle_manager {

class Service {
  public:
    Service();
    virtual ~Service();

    std::string GetName();
    ServiceType GetServiceType();
    RecoveryPolicy GetRecoveryPolicy();
    bool isEnabled();

    ServiceMode GetServiceMode();
    void SetServiceMode(ServiceMode mode);

    ServiceState GetServiceState();
    void SetServiceState(ServiceState state);
    ServiceState GetLastServiceState();    


    int32_t GetRecoverdCount();
    void IncreateRecoverdCount();
    void ClearRecoverdCount();


  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;

    ServiceMode service_mode_;
    ServiceState service_state_;
    ServiceState service_state_last_;
    uint32_t recovered_count_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_HPP_
