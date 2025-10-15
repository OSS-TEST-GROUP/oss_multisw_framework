#pragma once
#include <string>
#include "otam_common/idl_headers.h"

struct SessionResult {
    ResultCode   ret_code{ResultCode::UNKNOWN};
    std::string  session_id;
};

struct AppSettingsResult {
    ResultCode   ret_code{ResultCode::UNKNOWN};
    std::string  app_settings;  // JSON string
};

struct AppUpdateStateResult {
    ResultCode   ret_code{ResultCode::UNKNOWN};
    std::string  app_state_info; // JSON string
};
