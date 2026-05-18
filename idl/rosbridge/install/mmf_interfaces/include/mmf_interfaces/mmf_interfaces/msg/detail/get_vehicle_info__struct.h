// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/GetVehicleInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__STRUCT_H_

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
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/GetVehicleInfo in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamVehicleServiceMsg.idl
  * Module: OtaManager, Struct: GetVehicleInfo
 */
typedef struct mmf_interfaces__msg__GetVehicleInfo
{
  int32_t id;
  rosidl_runtime_c__String name;
} mmf_interfaces__msg__GetVehicleInfo;

// Struct for a sequence of mmf_interfaces__msg__GetVehicleInfo.
typedef struct mmf_interfaces__msg__GetVehicleInfo__Sequence
{
  mmf_interfaces__msg__GetVehicleInfo * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__GetVehicleInfo__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__STRUCT_H_
