#ifndef LIFECYCLE_CONTAINER_TYPE_HPP_
#define LIFECYCLE_CONTAINER_TYPE_HPP_

// F013: parity with IDL enum lifecyclemgr::ContainerType.
// Declaration order is wire-significant — int values are sent on
// UpdateServiceListRequest / NotifyContainerState — keep aligned with
// idl/dds/lifecycle/LifecycleCommon.idl.

namespace lifecycle_manager {

enum class ContainerType {
    CT_UNKNOWN,
    DOCKER,
    DOCKER_COMPOSE,
    K3S,
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_CONTAINER_TYPE_HPP_
