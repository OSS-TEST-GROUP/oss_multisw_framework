// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/NotifyServiceState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__STRUCT_H_

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
// Member 'service_state_list'
#include "rosidl_runtime_c/primitives_sequence.h"

/// Struct defined in msg/NotifyServiceState in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/NotifyServiceState.idl
  * Module: lifecyclemgr, Struct: NotifyServiceState
 */
typedef struct mmf_interfaces__msg__NotifyServiceState
{
  int32_t os_type;
  rosidl_runtime_c__String__Sequence service_list;
  rosidl_runtime_c__int32__Sequence service_state_list;
} mmf_interfaces__msg__NotifyServiceState;

// Struct for a sequence of mmf_interfaces__msg__NotifyServiceState.
typedef struct mmf_interfaces__msg__NotifyServiceState__Sequence
{
  mmf_interfaces__msg__NotifyServiceState * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__NotifyServiceState__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__STRUCT_H_
