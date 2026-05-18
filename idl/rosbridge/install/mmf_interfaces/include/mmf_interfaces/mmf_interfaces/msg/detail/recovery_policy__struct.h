// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/RecoveryPolicy.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'R_DEFAULT'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__R_DEFAULT = 0l
};

/// Constant 'ALWAYS'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__ALWAYS = 1l
};

/// Constant 'ON_FAILURE'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__ON_FAILURE = 2l
};

/// Constant 'ON_SUCCESS'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__ON_SUCCESS = 3l
};

/// Constant 'ON_NOT_RESPONDING'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__ON_NOT_RESPONDING = 4l
};

/// Constant 'MANUAL'.
enum
{
  mmf_interfaces__msg__RecoveryPolicy__MANUAL = 5l
};

/// Struct defined in msg/RecoveryPolicy in the package mmf_interfaces.
/**
  * RecoveryPolicy enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__RecoveryPolicy
{
  int32_t value;
} mmf_interfaces__msg__RecoveryPolicy;

// Struct for a sequence of mmf_interfaces__msg__RecoveryPolicy.
typedef struct mmf_interfaces__msg__RecoveryPolicy__Sequence
{
  mmf_interfaces__msg__RecoveryPolicy * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__RecoveryPolicy__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_H_
