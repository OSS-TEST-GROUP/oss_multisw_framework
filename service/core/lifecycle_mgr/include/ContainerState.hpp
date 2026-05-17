#ifndef LIFECYCLE_CONTAINER_STATE_HPP_
#define LIFECYCLE_CONTAINER_STATE_HPP_

// F013: parity with IDL enum lifecyclemgr::ContainerState
// (idl/dds/lifecycle/LifecycleCommon.idl). Declaration order is
// wire-significant.

namespace lifecycle_manager {

enum class ContainerState {
    CS_UNKNOWN,
    CONTAINER_STOPPED,
    CONTAINER_RUNNING,
    CONTAINER_PAUSED,
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CONTAINER_STATE_HPP_
