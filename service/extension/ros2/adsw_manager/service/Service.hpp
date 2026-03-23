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

  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;
    ServiceState service_state_;
    std::string packag_name_;
    std::string launch_file_;

    int32_t max_nodes_;
    int32_t current_nodes_;
    uint64_t pid_;
    int32_t exit_code_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
