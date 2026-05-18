// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/ReturnValue.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__RETURN_VALUE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__RETURN_VALUE__STRUCT_H_

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
  mmf_interfaces__msg__ReturnValue__UNKNOWN = 0l
};

/// Constant 'SUCCESS'.
enum
{
  mmf_interfaces__msg__ReturnValue__SUCCESS = 1l
};

/// Constant 'FAIL'.
enum
{
  mmf_interfaces__msg__ReturnValue__FAIL = 2l
};

/// Struct defined in msg/ReturnValue in the package mmf_interfaces.
/**
  * ReturnValue enum (from common/ReturnValue.idl)
 */
typedef struct mmf_interfaces__msg__ReturnValue
{
  int32_t value;
} mmf_interfaces__msg__ReturnValue;

// Struct for a sequence of mmf_interfaces__msg__ReturnValue.
typedef struct mmf_interfaces__msg__ReturnValue__Sequence
{
  mmf_interfaces__msg__ReturnValue * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__ReturnValue__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__RETURN_VALUE__STRUCT_H_
