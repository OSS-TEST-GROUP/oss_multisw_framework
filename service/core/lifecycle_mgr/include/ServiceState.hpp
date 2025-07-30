#ifndef LIFECYCLE_SERVICE_STATE_HPP_
#define LIFECYCLE_SERVICE_STATE_HPP_

namespace lifecycle_manager {

enum class ServiceState {
    NOT_RUNNING,
    INITIALIZING,
    RUNNING,
    RUNNING_NO_RESPONSE,
    TERMINATING
};


} // namespace lifecycle_manager

#endif // LIFECYCLE_SERVICE_STATE_HPP_
