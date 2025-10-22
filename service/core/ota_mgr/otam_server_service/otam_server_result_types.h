#ifndef OTAM_SERVER_RESULT_TYPES_H_
#define OTAM_SERVER_RESULT_TYPES_H_
#include <string>
#include "otam_common/otam_dds_idl.h"

struct SessionResult {
    ReturnValue   ret_code{ReturnValue::UNKNOWN};
    std::string  session_id;
};

struct AppSettingsResult {
    ReturnValue   ret_code{ReturnValue::UNKNOWN};
    std::string  app_settings;  // JSON string
};

struct AppUpdateStateResult {
    ReturnValue   ret_code{ReturnValue::UNKNOWN};
    std::string  app_state_info; // JSON string
};

#endif //OTAM_SERVER_RESULT_TYPES_H_