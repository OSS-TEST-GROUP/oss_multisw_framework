// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/DiagnosticInfoRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'diag_name'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/DiagnosticInfoRequest in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
  * Module: diagnosticmgr, Struct: DiagnosticInfoRequest
 */
typedef struct mmf_interfaces__msg__DiagnosticInfoRequest
{
  int32_t diag_code;
  rosidl_runtime_c__String diag_name;
} mmf_interfaces__msg__DiagnosticInfoRequest;

// Struct for a sequence of mmf_interfaces__msg__DiagnosticInfoRequest.
typedef struct mmf_interfaces__msg__DiagnosticInfoRequest__Sequence
{
  mmf_interfaces__msg__DiagnosticInfoRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__DiagnosticInfoRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__STRUCT_H_
