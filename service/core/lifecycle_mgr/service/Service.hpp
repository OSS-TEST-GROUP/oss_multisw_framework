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
    void SetServiceType(ServiceType type);
    RecoveryPolicy GetRecoveryPolicy();
    void SetRecoveryPolicy(RecoveryPolicy policy);
    bool isEnabled();
    void SetEnabled(bool enabled);

    ServiceMode GetServiceMode();
    void SetServiceMode(ServiceMode mode);

    ServiceState GetServiceState();
    void SetServiceState(ServiceState state);
    ServiceState GetLastServiceState();    

    int32_t GetRecoveredCount();
    void IncreaseRecoveredCount();
    void ClearRecoveredCount();

    // Exit code reported by the service agent when the process terminates.
    // -1 = never started (initial); 0 = clean exit; >0 = error exit.
    int32_t GetExitCode();
    void SetExitCode(int32_t code);

    void Print();

    OsType GetOsType();

    // F013: which container hosts this service.  Empty string means
    // "native / non-containerised" (treated as a distinct registration
    // group by the registry).  Mutable so a re-registration can move a
    // service between containers without dropping the Service object.
    const std::string& GetContainerName() const { return container_name_; }
    void SetContainerName(const std::string& name) { container_name_ = name; }

  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;

    ServiceMode service_mode_;
    ServiceState service_state_;
    ServiceState service_state_last_;
    uint32_t recovered_count_;
    int32_t exit_code_;

    OsType os_type_;

    std::string container_name_;  // F013

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_HPP_
