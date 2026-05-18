// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/AdswServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'status_msg'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/AdswServiceStatus in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ros2/AdswServiceStatus.idl
  * Module: adswmgr, Struct: AdswServiceStatus
 */
typedef struct mmf_interfaces__msg__AdswServiceStatus
{
  int32_t status_code;
  rosidl_runtime_c__String status_msg;
} mmf_interfaces__msg__AdswServiceStatus;

// Struct for a sequence of mmf_interfaces__msg__AdswServiceStatus.
typedef struct mmf_interfaces__msg__AdswServiceStatus__Sequence
{
  mmf_interfaces__msg__AdswServiceStatus * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__AdswServiceStatus__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_H_
