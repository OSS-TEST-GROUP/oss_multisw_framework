#ifndef ROS2_ADSW_MANAGER_SERIVCE_SERVICE_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_SERVICE_HPP_

#include <iostream>
#include <string>

#include "ServiceState.hpp"
#include "ServiceType.hpp"
#include "RecoveryPolicy.hpp"
#include "OsType.hpp"

namespace ros2 {
namespace adsw_manager {

class Service {
  public:
    Service(const std::string& name, ServiceType service_type, RecoveryPolicy recovery_policy, bool is_enable);
    virtual ~Service();

    std::string GetName();

    ServiceType GetServiceType();
    RecoveryPolicy GetRecoveryPolicy();
    // F014 follow-up: in-memory mutation for ctrl/SetRecoveryPolicy.
    // ServiceRegistry::Save() persists to saved-service-list.yaml.
    void SetRecoveryPolicy(RecoveryPolicy policy);
    bool IsEnabled();

    ServiceState GetServiceState();
    void SetServiceState(ServiceState state);

    void SetLaunchFile(const std::string& launchfile);
    const std::string GetLaunchFile(){return launch_file_;}

    void SetPackage(const std::string& package);
    const std::string GetPackage();
    void SetRunningNodes(int32_t count);
    int32_t GetRunningNodes();
    int32_t GetMaxNodes();

    void SetPID(uint64_t pid);
    uint64_t GetPID();

    void SetExitCode(int32_t code);
    int32_t GetExitCode();

    // Optional explicit token used by liveness monitoring to find this
    // service's process inside its container. Setting this overrides the
    // launchfile/name/package auto-derivation done in ResolveProcessMatchToken.
    // Use it for tricky cases like exec wrappers or fork-and-exit scripts
    // where the launching script's name is not what survives in /proc.
    void SetProcessMatchPattern(const std::string& pattern);
    const std::string& GetProcessMatchPattern() const;

    // True when the service is launched as a direct binary or script via
    //   `docker exec <c> bash -c "<package> & echo $!"`,
    // bypassing ros2 run / ros2 launch. The package field carries the
    // executable path or PATH-resolvable name. Set from YAML's
    // `exec_type: "binary"` (default false → ros2 run/launch).
    void SetDirectExec(bool direct);
    bool IsDirectExec() const;

    // F013: which container hosts this service. Empty string = native /
    // non-containerised. Set during ServiceRegistry::LoadServiceSaved from
    // the enclosing container's `name` field and shipped to lifecycle_mgr
    // in UpdateServiceListRequest.Service.container_name.
    void SetContainerName(const std::string& name) { container_name_ = name; }
    const std::string& GetContainerName() const { return container_name_; }

    // Pick the cmdline-suffix that uniquely identifies this service's process
    // in `pgrep -f <token>$`. Precedence:
    //   1. Explicit ProcessMatchPattern if set
    //   2. LaunchFile          (when ros2 launch is used)
    //   3. Package             (when direct binary/script is executed)
    //   4. Name                (when ros2 run is used — default)
    // is_native (legacy host path) and is_direct_exec_ (in-container direct
    // exec) both select branch 3. Either flag flips into the binary mode.
    std::string ResolveProcessMatchToken(bool is_native) const;

  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;
    ServiceState service_state_;
    std::string packag_name_;
    std::string launch_file_;
    std::string match_pattern_;
    bool is_direct_exec_;

    int32_t max_nodes_;
    int32_t current_nodes_;
    uint64_t pid_;
    int32_t exit_code_;

    std::string container_name_;  // F013
};

} // namespace adsw_manager
} // namespace ros2

#endif
