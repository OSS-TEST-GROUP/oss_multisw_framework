#ifndef OTA_MGR_COMMON_ENUM_H_
#define OTA_MGR_COMMON_ENUM_H_

#include "generated/idl/common/ReturnValue.hpp"

// OtamClientServiceMsg
#include "generated/idl/ota_mgr/OtamClientServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamClientServiceMsgPubSubTypes.hpp"
// OtamDianosticServiceMsg
#include "generated/idl/ota_mgr/OtamDiagnosticServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamDiagnosticServiceMsgPubSubTypes.hpp"
// F007 — NotifyOtaMgrHeartbeat (separate IDL, periodic heartbeat publish)
#include "generated/idl/ota_mgr/NotifyOtaMgrHeartbeat.hpp"
#include "generated/idl/ota_mgr/NotifyOtaMgrHeartbeatPubSubTypes.hpp"
// OtamVehicleServiceMsg
#include "generated/idl/ota_mgr/OtamVehicleServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamVehicleServiceMsgPubSubTypes.hpp"
// OtamVehicleServiceMsg
#include "generated/idl/ota_mgr/OtamActorServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamActorServiceMsgPubSubTypes.hpp"

// Common ReturnValue enum (assuming this is shared across the project)
// It's good practice to define this in a common header or namespace.
// enum ReturnValue {
//     kUnknown = 0,
//     kSuccess = 1,
//     kFail = 2,
//   };

#endif  // OTA_MGR_COMMON_ENUM_H_
