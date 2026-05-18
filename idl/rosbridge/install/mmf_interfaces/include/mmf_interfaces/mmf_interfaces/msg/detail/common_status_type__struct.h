// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/CommonStatusType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Constant 'UNKNOWN'.
enum
{
  mmf_interfaces__msg__CommonStatusType__UNKNOWN = 0l
};

/// Constant 'PASS'.
enum
{
  mmf_interfaces__msg__CommonStatusType__PASS = 1l
};

/// Constant 'FAIL'.
enum
{
  mmf_interfaces__msg__CommonStatusType__FAIL = 2l
};

/// Struct defined in msg/CommonStatusType in the package mmf_interfaces.
/**
  * CommonStatusType enum (from common/Common.idl)
 */
typedef struct mmf_interfaces__msg__CommonStatusType
{
  int32_t value;
} mmf_interfaces__msg__CommonStatusType;

// Struct for a sequence of mmf_interfaces__msg__CommonStatusType.
typedef struct mmf_interfaces__msg__CommonStatusType__Sequence
{
  mmf_interfaces__msg__CommonStatusType * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__CommonStatusType__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__STRUCT_H_
