// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/SetOtaClientRegistration.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'client_name'
// Member 'app_list'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/SetOtaClientRegistration in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
  * Module: OtaManager, Struct: SetOtaClientRegistration
 */
typedef struct mmf_interfaces__msg__SetOtaClientRegistration
{
  rosidl_runtime_c__String client_name;
  rosidl_runtime_c__String__Sequence app_list;
} mmf_interfaces__msg__SetOtaClientRegistration;

// Struct for a sequence of mmf_interfaces__msg__SetOtaClientRegistration.
typedef struct mmf_interfaces__msg__SetOtaClientRegistration__Sequence
{
  mmf_interfaces__msg__SetOtaClientRegistration * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__SetOtaClientRegistration__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_H_
