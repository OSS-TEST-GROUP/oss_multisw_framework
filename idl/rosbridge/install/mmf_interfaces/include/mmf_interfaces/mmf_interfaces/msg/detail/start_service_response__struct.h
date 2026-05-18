// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/StartServiceResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__START_SERVICE_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__START_SERVICE_RESPONSE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/StartServiceResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/lifecycle/StartServiceRequest.idl
  * Module: lifecyclemgr, Struct: StartServiceResponse
 */
typedef struct mmf_interfaces__msg__StartServiceResponse
{
  int32_t ret_code;
} mmf_interfaces__msg__StartServiceResponse;

// Struct for a sequence of mmf_interfaces__msg__StartServiceResponse.
typedef struct mmf_interfaces__msg__StartServiceResponse__Sequence
{
  mmf_interfaces__msg__StartServiceResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__StartServiceResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__START_SERVICE_RESPONSE__STRUCT_H_
