// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/ServiceType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SERVICE_TYPE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SERVICE_TYPE__STRUCT_H_

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
  mmf_interfaces__msg__ServiceType__NORMAL = 0l
};

/// Constant 'ONESHOT'.
enum
{
  mmf_interfaces__msg__ServiceType__ONESHOT = 1l
};

/// Struct defined in msg/ServiceType in the package mmf_interfaces.
/**
  * ServiceType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__ServiceType
{
  int32_t value;
} mmf_interfaces__msg__ServiceType;

// Struct for a sequence of mmf_interfaces__msg__ServiceType.
typedef struct mmf_interfaces__msg__ServiceType__Sequence
{
  mmf_interfaces__msg__ServiceType * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__ServiceType__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SERVICE_TYPE__STRUCT_H_
