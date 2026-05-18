// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mmf_interfaces:msg/GetAppUpdateStateResponse.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mmf_interfaces/msg/detail/get_app_update_state_response__rosidl_typesupport_introspection_c.h"
#include "mmf_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mmf_interfaces/msg/detail/get_app_update_state_response__functions.h"
#include "mmf_interfaces/msg/detail/get_app_update_state_response__struct.h"


// Include directives for member types
// Member `app_name`
// Member `app_state_info`
#include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mmf_interfaces__msg__GetAppUpdateStateResponse__init(message_memory);
}

void mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_fini_function(void * message_memory)
{
  mmf_interfaces__msg__GetAppUpdateStateResponse__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_member_array[4] = {
  {
    "id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__GetAppUpdateStateResponse, id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "app_name",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__GetAppUpdateStateResponse, app_name),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "app_state_info",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__GetAppUpdateStateResponse, app_state_info),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "ret_code",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__GetAppUpdateStateResponse, ret_code),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_members = {
  "mmf_interfaces__msg",  // message namespace
  "GetAppUpdateStateResponse",  // message name
  4,  // number of fields
  sizeof(mmf_interfaces__msg__GetAppUpdateStateResponse),
  mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_member_array,  // message members
  mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_init_function,  // function to initialize message memory (memory has to be allocated)
  mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_type_support_handle = {
  0,
  &mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mmf_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, GetAppUpdateStateResponse)() {
  if (!mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_type_support_handle.typesupport_identifier) {
    mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mmf_interfaces__msg__GetAppUpdateStateResponse__rosidl_typesupport_introspection_c__GetAppUpdateStateResponse_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
