// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/SetServiceModeRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'service_list'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/SetServiceModeRequest in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/SetServiceModeRequest.idl
  * Module: lifecyclemgr, Struct: SetServiceModeRequest
 */
typedef struct mmf_interfaces__msg__SetServiceModeRequest
{
  int32_t os_type;
  rosidl_runtime_c__String__Sequence service_list;
  bool is_group;
  int32_t service_mode;
} mmf_interfaces__msg__SetServiceModeRequest;

// Struct for a sequence of mmf_interfaces__msg__SetServiceModeRequest.
typedef struct mmf_interfaces__msg__SetServiceModeRequest__Sequence
{
  mmf_interfaces__msg__SetServiceModeRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__SetServiceModeRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_H_
