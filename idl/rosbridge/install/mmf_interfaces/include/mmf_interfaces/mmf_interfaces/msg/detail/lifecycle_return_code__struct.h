// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/LifecycleReturnCode.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'SUCCESS'.
enum
{
  mmf_interfaces__msg__LifecycleReturnCode__SUCCESS = 0l
};

/// Constant 'FAILED'.
enum
{
  mmf_interfaces__msg__LifecycleReturnCode__FAILED = 1l
};

/// Struct defined in msg/LifecycleReturnCode in the package mmf_interfaces.
/**
  * ReturnCode enum for lifecycle (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__LifecycleReturnCode
{
  int32_t value;
} mmf_interfaces__msg__LifecycleReturnCode;

// Struct for a sequence of mmf_interfaces__msg__LifecycleReturnCode.
typedef struct mmf_interfaces__msg__LifecycleReturnCode__Sequence
{
  mmf_interfaces__msg__LifecycleReturnCode * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__LifecycleReturnCode__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__STRUCT_H_
