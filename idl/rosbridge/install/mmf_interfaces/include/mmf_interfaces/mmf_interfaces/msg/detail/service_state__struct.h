// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/ServiceState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SERVICE_STATE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SERVICE_STATE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'NOT_RUNNING'.
enum
{
  mmf_interfaces__msg__ServiceState__NOT_RUNNING = 0l
};

/// Constant 'INITIALIZING'.
enum
{
  mmf_interfaces__msg__ServiceState__INITIALIZING = 1l
};

/// Constant 'RUNNING'.
enum
{
  mmf_interfaces__msg__ServiceState__RUNNING = 2l
};

/// Constant 'RUNNING_NO_RESPONSE'.
enum
{
  mmf_interfaces__msg__ServiceState__RUNNING_NO_RESPONSE = 3l
};

/// Constant 'TERMINATING'.
enum
{
  mmf_interfaces__msg__ServiceState__TERMINATING = 4l
};

/// Struct defined in msg/ServiceState in the package mmf_interfaces.
/**
  * ServiceState enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__ServiceState
{
  int32_t value;
} mmf_interfaces__msg__ServiceState;

// Struct for a sequence of mmf_interfaces__msg__ServiceState.
typedef struct mmf_interfaces__msg__ServiceState__Sequence
{
  mmf_interfaces__msg__ServiceState * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__ServiceState__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SERVICE_STATE__STRUCT_H_
