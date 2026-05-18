// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/DiagnosticInfoListRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/DiagnosticInfoListRequest in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/diagnostic/DiagnosticInfo.idl
  * Module: diagnosticmgr, Struct: DiagnosticInfoListRequest
  * Empty message
 */
typedef struct mmf_interfaces__msg__DiagnosticInfoListRequest
{
  uint8_t structure_needs_at_least_one_member;
} mmf_interfaces__msg__DiagnosticInfoListRequest;

// Struct for a sequence of mmf_interfaces__msg__DiagnosticInfoListRequest.
typedef struct mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence
{
  mmf_interfaces__msg__DiagnosticInfoListRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__STRUCT_H_
