// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/NotifyOtaMgrInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'err_msg'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/NotifyOtaMgrInfo in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamDiagnosticServiceMsg.idl
  * Module: OtaManager, Struct: NotifyOtaMgrInfo
 */
typedef struct mmf_interfaces__msg__NotifyOtaMgrInfo
{
  int32_t err_code;
  rosidl_runtime_c__String err_msg;
} mmf_interfaces__msg__NotifyOtaMgrInfo;

// Struct for a sequence of mmf_interfaces__msg__NotifyOtaMgrInfo.
typedef struct mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence
{
  mmf_interfaces__msg__NotifyOtaMgrInfo * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__NotifyOtaMgrInfo__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_H_
