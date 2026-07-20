#ifndef GENERAL_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_
#define GENERAL_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_

namespace general {
namespace adsw_manager {
    

    enum class ServiceState {
        NOT_RUNNING,
        INITIALIZING,
        RUNNING,
        RUNNING_NO_RESPONSE,
        TERMINATING
    };    
} // namespace adsw_manager
} // namespace general

#endif // GENERAL_ADSW_MANAGER_SERIVCE_SERVICE_STATE_HPP_
