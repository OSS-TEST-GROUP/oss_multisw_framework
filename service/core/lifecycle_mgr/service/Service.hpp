#ifndef LIFECYCLE_SERVICE_HPP_
#define LIFECYCLE_SERVICE_HPP_

#include <string>

#include "include/ServiceState.hpp"
#include "include/ServiceType.hpp"
#include "include/ServiceMode.hpp"
#include "include/RecoveryPolicy.hpp"
#include "include/OsType.hpp"

namespace lifecycle_manager {

class Service {

  public:
    Service();
    Service(const std::string& name, const ServiceType type, const RecoveryPolicy policy, const bool enabled, const OsType os);
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

    int32_t GetRecoveredCount();
    void IncreaseRecoveredCount();
    void ClearRecoveredCount();

    void Print();

    OsType GetOsType();

  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;

    ServiceMode service_mode_;
    ServiceState service_state_;
    ServiceState service_state_last_;
    uint32_t recovered_count_;

    OsType os_type_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_HPP_
