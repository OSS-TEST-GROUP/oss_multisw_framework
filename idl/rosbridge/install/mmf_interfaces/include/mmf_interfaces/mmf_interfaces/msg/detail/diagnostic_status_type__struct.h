// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/DiagnosticStatusType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_H_

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
  mmf_interfaces__msg__DiagnosticStatusType__UNKNOWN = 0l
};

/// Constant 'PASS'.
enum
{
  mmf_interfaces__msg__DiagnosticStatusType__PASS = 1l
};

/// Constant 'FATAL'.
enum
{
  mmf_interfaces__msg__DiagnosticStatusType__FATAL = 2l
};

/// Constant 'WARNING'.
enum
{
  mmf_interfaces__msg__DiagnosticStatusType__WARNING = 3l
};

/// Struct defined in msg/DiagnosticStatusType in the package mmf_interfaces.
/**
  * DiagnosticStatusType enum (from diagnostic/DiagnosticCommon.idl :: diagnosticmgr module)
 */
typedef struct mmf_interfaces__msg__DiagnosticStatusType
{
  int32_t value;
} mmf_interfaces__msg__DiagnosticStatusType;

// Struct for a sequence of mmf_interfaces__msg__DiagnosticStatusType.
typedef struct mmf_interfaces__msg__DiagnosticStatusType__Sequence
{
  mmf_interfaces__msg__DiagnosticStatusType * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__DiagnosticStatusType__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_H_
