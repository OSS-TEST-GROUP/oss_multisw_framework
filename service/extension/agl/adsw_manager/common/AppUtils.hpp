#ifndef APP_UTILS_HPP
#define APP_UTILS_HPP

#include <string>
#include <ostream>

#include "LifecycleCommon.hpp"

namespace adsw_manager::agl {

    inline std::ostream& operator<<(std::ostream& os, lifecyclemgr::ServiceState state) {
        switch (state) {
            case lifecyclemgr::ServiceState::NOT_RUNNING:
                os << "NOT_RUNNING";
                break;
            case lifecyclemgr::ServiceState::INITIALIZING:
                os << "INITIALIZING";
                break;
            case lifecyclemgr::ServiceState::RUNNING:
                os << "RUNNING";
                break;
            case lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE:
                os << "RUNNING_NO_RESPONSE";
                break;
            case lifecyclemgr::ServiceState::TERMINATING:
                os << "TERMINATING";
                break;
            default:
                os << "INVALID_STATE";
                break;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, lifecyclemgr::ReturnCode code) {
        switch (code) {
            case lifecyclemgr::ReturnCode::SUCCESS:
                os << "SUCCESS";
                break;
            case lifecyclemgr::ReturnCode::FAILED:
                os << "FAILED";
                break;
            default:
                os << "UNKNOWN_CODE";
                break;
        }
        return os;
    }


} // namespace adsw_manager::agl

#endif