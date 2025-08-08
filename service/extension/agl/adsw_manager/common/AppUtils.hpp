#ifndef APP_UTILS_HPP
#define APP_UTILS_HPP

#include <string>
#include <ostream>

#include "AppInfo.hpp"

namespace adsw_manager::agl {

    inline std::ostream& operator<<(std::ostream& os, adsw_manager_agl::AppState state) {
        switch (state) {
            case adsw_manager_agl::AppState::UNKNOWN:
                os << "UNKNOWN";
                break;
            case adsw_manager_agl::AppState::PAUSED:
                os << "PAUSED";
                break;
            case adsw_manager_agl::AppState::RESUMED:
                os << "RESUMED";
                break;
            case adsw_manager_agl::AppState::INACTIVE:
                os << "INACTIVE";
                break;
            case adsw_manager_agl::AppState::DETACHED:
                os << "DETACHED";
                break;
            default:
                os << "INVALID_STATE";
                break;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, adsw_manager_agl::ReturnCode code) {
        switch (code) {
            case adsw_manager_agl::ReturnCode::SUCCESS:
                os << "SUCCESS";
                break;
            case adsw_manager_agl::ReturnCode::FAILED:
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