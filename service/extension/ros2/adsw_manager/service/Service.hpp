#ifndef ROS2_ADSW_MANAGER_SERIVCE_SERVICE_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_SERVICE_HPP_

#include <iostream>
#include <string>

#include "ServiceState.hpp"
#include "ServiceType.hpp"
#include "RecoveryPolicy.hpp"

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

  private:
    std::string name_;
    ServiceType service_type_;
    RecoveryPolicy recovery_policy_;
    bool is_enabled_;
    ServiceState service_state_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
