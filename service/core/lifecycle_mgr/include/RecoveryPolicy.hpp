#ifndef LIFECYCLE_RECOVERY_POLICY_HPP_
#define LIFECYCLE_RECOVERY_POLICY_HPP_

namespace lifecycle_manager {

enum class RecoveryPolicy {
    DEFAULT,
    ALWAYS,
    ON_FAILURE,
    ON_SUCCESS,
    ON_NOT_RESPONDING,
    MANUAL
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_RECOVERY_POLICY_HPP_
