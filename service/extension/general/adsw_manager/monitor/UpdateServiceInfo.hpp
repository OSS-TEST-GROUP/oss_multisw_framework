#ifndef GENERAL_ADSW_MANAGER_MONITOR_UPDATE_SERVICE_INFO_HPP_
#define GENERAL_ADSW_MANAGER_MONITOR_UPDATE_SERVICE_INFO_HPP_

#include <iostream>
#include <string>

#include "service/ServiceState.hpp"

namespace general {
namespace adsw_manager {

enum class ServiceEvent {
    CREATED,
    DELETED,
    STATE_UPDATED,
};

class UpdateServiceInfo {
  public:
    UpdateServiceInfo(const std::string& name, const std::string& namespace_name, ServiceEvent event, ServiceState state);
    std::string name;
    std::string namespace_name;
    ServiceEvent service_event;
    ServiceState state;
};

} // namespace adsw_manager
} // namespace general

#endif
