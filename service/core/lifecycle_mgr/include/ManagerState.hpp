#ifndef LIFECYCLE_MANAGER_STATE_HPP_
#define LIFECYCLE_MANAGER_STATE_HPP_

namespace lifecycle_manager {

enum class ManagerState {
    MANAGER_INIT,
    MANAGER_STARTED,
    MANAGER_RUNNING,
    MANAGER_PASUE,
    MANAGER_TERMINATE
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MANAGER_STATE_HPP_
