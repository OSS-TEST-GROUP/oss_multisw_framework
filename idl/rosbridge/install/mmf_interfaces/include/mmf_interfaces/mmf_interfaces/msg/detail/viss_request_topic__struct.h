// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/VissRequestTopic.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'payload'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/VissRequestTopic in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/vehicle_info_mgr/VissRequest.idl
  * Module: , Struct: VissRequestTopic
 */
typedef struct mmf_interfaces__msg__VissRequestTopic
{
  rosidl_runtime_c__String payload;
} mmf_interfaces__msg__VissRequestTopic;

// Struct for a sequence of mmf_interfaces__msg__VissRequestTopic.
typedef struct mmf_interfaces__msg__VissRequestTopic__Sequence
{
  mmf_interfaces__msg__VissRequestTopic * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__VissRequestTopic__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__STRUCT_H_
