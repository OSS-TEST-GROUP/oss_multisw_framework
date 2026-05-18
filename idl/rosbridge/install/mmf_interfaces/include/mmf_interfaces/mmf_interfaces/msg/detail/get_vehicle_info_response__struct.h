// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/GetVehicleInfoResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'name'
// Member 'value'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/GetVehicleInfoResponse in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
  * Module: OtaManager, Struct: GetVehicleInfoResponse
 */
typedef struct mmf_interfaces__msg__GetVehicleInfoResponse
{
  int32_t id;
  rosidl_runtime_c__String name;
  rosidl_runtime_c__String value;
  int32_t ret_code;
} mmf_interfaces__msg__GetVehicleInfoResponse;

// Struct for a sequence of mmf_interfaces__msg__GetVehicleInfoResponse.
typedef struct mmf_interfaces__msg__GetVehicleInfoResponse__Sequence
{
  mmf_interfaces__msg__GetVehicleInfoResponse * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__GetVehicleInfoResponse__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_H_
