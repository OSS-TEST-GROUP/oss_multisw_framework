// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mmf_interfaces:msg/SetServiceModeRequest.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mmf_interfaces/msg/detail/set_service_mode_request__rosidl_typesupport_introspection_c.h"
#include "mmf_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mmf_interfaces/msg/detail/set_service_mode_request__functions.h"
#include "mmf_interfaces/msg/detail/set_service_mode_request__struct.h"


// Include directives for member types
// Member `service_list`
#include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mmf_interfaces__msg__SetServiceModeRequest__init(message_memory);
}

void mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_fini_function(void * message_memory)
{
  mmf_interfaces__msg__SetServiceModeRequest__fini(message_memory);
}

size_t mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__size_function__SetServiceModeRequest__service_list(
  const void * untyped_member)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return member->size;
}

const void * mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_const_function__SetServiceModeRequest__service_list(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_function__SetServiceModeRequest__service_list(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__fetch_function__SetServiceModeRequest__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const rosidl_runtime_c__String * item =
    ((const rosidl_runtime_c__String *)
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_const_function__SetServiceModeRequest__service_list(untyped_member, index));
  rosidl_runtime_c__String * value =
    (rosidl_runtime_c__String *)(untyped_value);
  *value = *item;
}

void mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__assign_function__SetServiceModeRequest__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  rosidl_runtime_c__String * item =
    ((rosidl_runtime_c__String *)
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_function__SetServiceModeRequest__service_list(untyped_member, index));
  const rosidl_runtime_c__String * value =
    (const rosidl_runtime_c__String *)(untyped_value);
  *item = *value;
}

bool mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__resize_function__SetServiceModeRequest__service_list(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  rosidl_runtime_c__String__Sequence__fini(member);
  return rosidl_runtime_c__String__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_member_array[4] = {
  {
    "os_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__SetServiceModeRequest, os_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "service_list",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__SetServiceModeRequest, service_list),  // bytes offset in struct
    NULL,  // default value
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__size_function__SetServiceModeRequest__service_list,  // size() function pointer
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_const_function__SetServiceModeRequest__service_list,  // get_const(index) function pointer
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__get_function__SetServiceModeRequest__service_list,  // get(index) function pointer
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__fetch_function__SetServiceModeRequest__service_list,  // fetch(index, &value) function pointer
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__assign_function__SetServiceModeRequest__service_list,  // assign(index, value) function pointer
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__resize_function__SetServiceModeRequest__service_list  // resize(index) function pointer
  },
  {
    "is_group",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__SetServiceModeRequest, is_group),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "service_mode",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__SetServiceModeRequest, service_mode),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_members = {
  "mmf_interfaces__msg",  // message namespace
  "SetServiceModeRequest",  // message name
  4,  // number of fields
  sizeof(mmf_interfaces__msg__SetServiceModeRequest),
  mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_member_array,  // message members
  mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_init_function,  // function to initialize message memory (memory has to be allocated)
  mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_type_support_handle = {
  0,
  &mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mmf_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, SetServiceModeRequest)() {
  if (!mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_type_support_handle.typesupport_identifier) {
    mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mmf_interfaces__msg__SetServiceModeRequest__rosidl_typesupport_introspection_c__SetServiceModeRequest_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
