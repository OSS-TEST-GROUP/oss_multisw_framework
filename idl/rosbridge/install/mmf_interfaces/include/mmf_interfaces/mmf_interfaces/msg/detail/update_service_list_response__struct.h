// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/UpdateServiceListResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_RESPONSE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/UpdateServiceListResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/UpdateServiceListRequest.idl
  * Module: lifecyclemgr, Struct: UpdateServiceListResponse
 */
typedef struct mmf_interfaces__msg__UpdateServiceListResponse
{
  int32_t ret_code;
} mmf_interfaces__msg__UpdateServiceListResponse;

// Struct for a sequence of mmf_interfaces__msg__UpdateServiceListResponse.
typedef struct mmf_interfaces__msg__UpdateServiceListResponse__Sequence
{
  mmf_interfaces__msg__UpdateServiceListResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__UpdateServiceListResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_RESPONSE__STRUCT_H_
