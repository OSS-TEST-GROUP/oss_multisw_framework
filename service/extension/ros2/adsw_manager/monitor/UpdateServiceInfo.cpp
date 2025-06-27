#include "UpdateServiceInfo.hpp"

using namespace lifecycle_manager;

namespace ros2 {
namespace adsw_manager {

UpdateServiceInfo::UpdateServiceInfo(const std::string& name, const std::string& namespace_name, ServiceEvent event, ServiceState state)
        : name(name), namespace_name(namespace_name), event(event), state(state) {}

} // namespace diagnostic_manager
} // namespace ros2
