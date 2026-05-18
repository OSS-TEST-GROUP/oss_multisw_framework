// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mmf_interfaces:msg/SetAppUpdate.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__STRUCT_H_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'app_name'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/SetAppUpdate in the package mmf_interfaces.
/**
  * Auto-generated from idl/dds/ota_mgr/OtamActorServiceMsg.idl
  * Module: OtaManager, Struct: SetAppUpdate
 */
typedef struct mmf_interfaces__msg__SetAppUpdate
{
  int32_t id;
  rosidl_runtime_c__String app_name;
} mmf_interfaces__msg__SetAppUpdate;

// Struct for a sequence of mmf_interfaces__msg__SetAppUpdate.
typedef struct mmf_interfaces__msg__SetAppUpdate__Sequence
{
  mmf_interfaces__msg__SetAppUpdate * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mmf_interfaces__msg__SetAppUpdate__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__STRUCT_H_
