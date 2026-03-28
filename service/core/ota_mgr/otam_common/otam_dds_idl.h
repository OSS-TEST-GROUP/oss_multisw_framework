#ifndef OTA_MGR_COMMON_ENUM_H_
#define OTA_MGR_COMMON_ENUM_H_

#include "generated/idl/common/ReturnValue.hpp"

// OtamClientServiceMsg
#include "generated/idl/ota_mgr/OtamClientServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamClientServiceMsgPubSubTypes.hpp"
// OtamDianosticServiceMsg
#include "generated/idl/ota_mgr/OtamDiagnosticServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamDiagnosticServiceMsgPubSubTypes.hpp"
// OtamVehicleServiceMsg
#include "generated/idl/ota_mgr/OtamVehicleServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamVehicleServiceMsgPubSubTypes.hpp"
// OtamVehicleServiceMsg
#include "generated/idl/ota_mgr/OtamActorServiceMsg.hpp"
#include "generated/idl/ota_mgr/OtamActorServiceMsgPubSubTypes.hpp"

// Namespace alias: IDL module was renamed from OtaManager to ota_manager::msg
// to match ROS2/rosbridge DDS type naming convention.
// This alias allows all existing C++ code using OtaManager:: to compile unchanged.
namespace OtaManager = ota_manager::msg;

#endif  // OTA_MGR_COMMON_ENUM_H_
