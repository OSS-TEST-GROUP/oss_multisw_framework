// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/NotifyAppUpdateStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'app_name'
// Member 'status'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/NotifyAppUpdateStatus in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
  * Module: OtaManager, Struct: NotifyAppUpdateStatus
 */
typedef struct mmf_interfaces__msg__NotifyAppUpdateStatus
{
  int32_t client_id;
  rosidl_runtime_c__String app_name;
  rosidl_runtime_c__String status;
} mmf_interfaces__msg__NotifyAppUpdateStatus;

// Struct for a sequence of mmf_interfaces__msg__NotifyAppUpdateStatus.
typedef struct mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence
{
  mmf_interfaces__msg__NotifyAppUpdateStatus * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__NotifyAppUpdateStatus__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_H_
