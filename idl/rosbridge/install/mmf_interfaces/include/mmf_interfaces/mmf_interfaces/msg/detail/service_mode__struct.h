// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/ServiceMode.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SERVICE_MODE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SERVICE_MODE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'NORMAL'.
enum
{
  mmf_interfaces__msg__ServiceMode__NORMAL = 0l
};

/// Constant 'OTA_MODE'.
enum
{
  mmf_interfaces__msg__ServiceMode__OTA_MODE = 1l
};

/// Struct defined in msg/ServiceMode in the package mmf_interfaces.
/**
  * ServiceMode enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__ServiceMode
{
  int32_t value;
} mmf_interfaces__msg__ServiceMode;

// Struct for a sequence of mmf_interfaces__msg__ServiceMode.
typedef struct mmf_interfaces__msg__ServiceMode__Sequence
{
  mmf_interfaces__msg__ServiceMode * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__ServiceMode__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SERVICE_MODE__STRUCT_H_
