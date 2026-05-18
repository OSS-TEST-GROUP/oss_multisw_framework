// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/GetAppUpdateStateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_H_

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
// Member 'app_state_info'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/GetAppUpdateStateResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
  * Module: OtaManager, Struct: GetAppUpdateStateResponse
 */
typedef struct mmf_interfaces__msg__GetAppUpdateStateResponse
{
  int32_t id;
  rosidl_runtime_c__String app_name;
  rosidl_runtime_c__String app_state_info;
  int32_t ret_code;
} mmf_interfaces__msg__GetAppUpdateStateResponse;

// Struct for a sequence of mmf_interfaces__msg__GetAppUpdateStateResponse.
typedef struct mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence
{
  mmf_interfaces__msg__GetAppUpdateStateResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__GetAppUpdateStateResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_H_
