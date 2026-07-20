#include "UpdateServiceInfo.hpp"


namespace general {
namespace adsw_manager {

UpdateServiceInfo::UpdateServiceInfo(const std::string& name, const std::string& namespace_name, ServiceEvent event, ServiceState state)
        : name(name), namespace_name(namespace_name), service_event(event), state(state) {



        }

} // namespace adsw_manager
} // namespace general
