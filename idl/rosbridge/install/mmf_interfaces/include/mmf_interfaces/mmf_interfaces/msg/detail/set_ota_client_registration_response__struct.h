// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/SetOtaClientRegistrationResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/SetOtaClientRegistrationResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamClientServiceMsg.idl
  * Module: OtaManager, Struct: SetOtaClientRegistrationResponse
 */
typedef struct mmf_interfaces__msg__SetOtaClientRegistrationResponse
{
  int32_t client_id;
  int32_t ret_code;
} mmf_interfaces__msg__SetOtaClientRegistrationResponse;

// Struct for a sequence of mmf_interfaces__msg__SetOtaClientRegistrationResponse.
typedef struct mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence
{
  mmf_interfaces__msg__SetOtaClientRegistrationResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__SetOtaClientRegistrationResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__STRUCT_H_
