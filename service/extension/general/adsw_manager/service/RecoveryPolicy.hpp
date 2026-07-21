#ifndef GENERAL_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_
#define GENERAL_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_

namespace general {
namespace adsw_manager {

    enum class RecoveryPolicy {
        R_DEFAULT,
        ALWAYS,
        ON_FAILURE,
        ON_SUCCESS,
        ON_NOT_RESPONDING,
        MANUAL
    };

} // namespace adsw_manager
} // namespace general

#endif // GENERAL_ADSW_MANAGER_SERIVCE_RECOVERY_POLICY_HPP_
