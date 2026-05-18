// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/DiagnosticInfoListResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'diag_codes'
#include "rosidl_runtime_c/primitives_sequence.h"
// Member 'diag_names'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/DiagnosticInfoListResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
  * Module: diagnosticmgr, Struct: DiagnosticInfoListResponse
 */
typedef struct mmf_interfaces__msg__DiagnosticInfoListResponse
{
  int32_t ret_code;
  rosidl_runtime_c__int32__Sequence diag_codes;
  rosidl_runtime_c__String__Sequence diag_names;
} mmf_interfaces__msg__DiagnosticInfoListResponse;

// Struct for a sequence of mmf_interfaces__msg__DiagnosticInfoListResponse.
typedef struct mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence
{
  mmf_interfaces__msg__DiagnosticInfoListResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__STRUCT_H_
