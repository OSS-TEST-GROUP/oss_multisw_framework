// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/OsType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__OS_TYPE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__OS_TYPE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'ROS2'.
enum
{
  mmf_interfaces__msg__OsType__ROS2 = 0l
};

/// Constant 'AGL'.
enum
{
  mmf_interfaces__msg__OsType__AGL = 1l
};

/// Constant 'AUTOSAR'.
enum
{
  mmf_interfaces__msg__OsType__AUTOSAR = 2l
};

/// Struct defined in msg/OsType in the package mmf_interfaces.
/**
  * OsType enum (from lifecycle/LifecycleCommon.idl :: lifecyclemgr module)
 */
typedef struct mmf_interfaces__msg__OsType
{
  int32_t value;
} mmf_interfaces__msg__OsType;

// Struct for a sequence of mmf_interfaces__msg__OsType.
typedef struct mmf_interfaces__msg__OsType__Sequence
{
  mmf_interfaces__msg__OsType * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__OsType__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__OS_TYPE__STRUCT_H_
