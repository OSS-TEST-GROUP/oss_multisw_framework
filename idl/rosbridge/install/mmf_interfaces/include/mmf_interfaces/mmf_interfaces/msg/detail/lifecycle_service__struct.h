// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/LifecycleService.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'name'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/LifecycleService in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
  * Module: lifecyclemgr, Struct: LifecycleService
 */
typedef struct mmf_interfaces__msg__LifecycleService
{
  rosidl_runtime_c__String name;
  int32_t service_type;
  int32_t recovery_policy;
  bool is_enabled;
  int32_t os_type;
} mmf_interfaces__msg__LifecycleService;

// Struct for a sequence of mmf_interfaces__msg__LifecycleService.
typedef struct mmf_interfaces__msg__LifecycleService__Sequence
{
  mmf_interfaces__msg__LifecycleService * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__LifecycleService__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_H_
