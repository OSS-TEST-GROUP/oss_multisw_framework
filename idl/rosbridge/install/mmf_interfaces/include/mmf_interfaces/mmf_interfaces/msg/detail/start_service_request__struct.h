// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/StartServiceRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__START_SERVICE_REQUEST__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__START_SERVICE_REQUEST__STRUCT_H_

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

/// Struct defined in msg/StartServiceRequest in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
  * Module: lifecyclemgr, Struct: StartServiceRequest
 */
typedef struct mmf_interfaces__msg__StartServiceRequest
{
  int32_t os_type;
  rosidl_runtime_c__String__Sequence service_list;
  bool is_group;
} mmf_interfaces__msg__StartServiceRequest;

// Struct for a sequence of mmf_interfaces__msg__StartServiceRequest.
typedef struct mmf_interfaces__msg__StartServiceRequest__Sequence
{
  mmf_interfaces__msg__StartServiceRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__StartServiceRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__START_SERVICE_REQUEST__STRUCT_H_
