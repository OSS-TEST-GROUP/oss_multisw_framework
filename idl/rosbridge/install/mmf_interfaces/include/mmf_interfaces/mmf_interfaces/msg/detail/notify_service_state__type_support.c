// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mmf_interfaces:msg/NotifyServiceState.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mmf_interfaces/msg/detail/notify_service_state__rosidl_typesupport_introspection_c.h"
#include "mmf_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mmf_interfaces/msg/detail/notify_service_state__functions.h"
#include "mmf_interfaces/msg/detail/notify_service_state__struct.h"


// Include directives for member types
// Member `service_list`
#include "rosidl_runtime_c/string_functions.h"
// Member `service_state_list`
#include "rosidl_runtime_c/primitives_sequence_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mmf_interfaces__msg__NotifyServiceState__init(message_memory);
}

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_fini_function(void * message_memory)
{
  mmf_interfaces__msg__NotifyServiceState__fini(message_memory);
}

size_t mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__size_function__NotifyServiceState__service_list(
  const void * untyped_member)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return member->size;
}

const void * mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_list(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__String__Sequence * member =
    (const rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_list(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  return &member->data[index];
}

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__fetch_function__NotifyServiceState__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const rosidl_runtime_c__String * item =
    ((const rosidl_runtime_c__String *)
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_list(untyped_member, index));
  rosidl_runtime_c__String * value =
    (rosidl_runtime_c__String *)(untyped_value);
  *value = *item;
}

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__assign_function__NotifyServiceState__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  rosidl_runtime_c__String * item =
    ((rosidl_runtime_c__String *)
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_list(untyped_member, index));
  const rosidl_runtime_c__String * value =
    (const rosidl_runtime_c__String *)(untyped_value);
  *item = *value;
}

bool mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__resize_function__NotifyServiceState__service_list(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__String__Sequence * member =
    (rosidl_runtime_c__String__Sequence *)(untyped_member);
  rosidl_runtime_c__String__Sequence__fini(member);
  return rosidl_runtime_c__String__Sequence__init(member, size);
}

size_t mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__size_function__NotifyServiceState__service_state_list(
  const void * untyped_member)
{
  const rosidl_runtime_c__int32__Sequence * member =
    (const rosidl_runtime_c__int32__Sequence *)(untyped_member);
  return member->size;
}

const void * mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_state_list(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__int32__Sequence * member =
    (const rosidl_runtime_c__int32__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_state_list(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__int32__Sequence * member =
    (rosidl_runtime_c__int32__Sequence *)(untyped_member);
  return &member->data[index];
}

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__fetch_function__NotifyServiceState__service_state_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const int32_t * item =
    ((const int32_t *)
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_state_list(untyped_member, index));
  int32_t * value =
    (int32_t *)(untyped_value);
  *value = *item;
}

void mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__assign_function__NotifyServiceState__service_state_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  int32_t * item =
    ((int32_t *)
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_state_list(untyped_member, index));
  const int32_t * value =
    (const int32_t *)(untyped_value);
  *item = *value;
}

bool mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__resize_function__NotifyServiceState__service_state_list(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__int32__Sequence * member =
    (rosidl_runtime_c__int32__Sequence *)(untyped_member);
  rosidl_runtime_c__int32__Sequence__fini(member);
  return rosidl_runtime_c__int32__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_member_array[3] = {
  {
    "os_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__NotifyServiceState, os_type),  // bytes offset in struct
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
    offsetof(mmf_interfaces__msg__NotifyServiceState, service_list),  // bytes offset in struct
    NULL,  // default value
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__size_function__NotifyServiceState__service_list,  // size() function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_list,  // get_const(index) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_list,  // get(index) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__fetch_function__NotifyServiceState__service_list,  // fetch(index, &value) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__assign_function__NotifyServiceState__service_list,  // assign(index, value) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__resize_function__NotifyServiceState__service_list  // resize(index) function pointer
  },
  {
    "service_state_list",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__NotifyServiceState, service_state_list),  // bytes offset in struct
    NULL,  // default value
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__size_function__NotifyServiceState__service_state_list,  // size() function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_const_function__NotifyServiceState__service_state_list,  // get_const(index) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__get_function__NotifyServiceState__service_state_list,  // get(index) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__fetch_function__NotifyServiceState__service_state_list,  // fetch(index, &value) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__assign_function__NotifyServiceState__service_state_list,  // assign(index, value) function pointer
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__resize_function__NotifyServiceState__service_state_list  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_members = {
  "mmf_interfaces__msg",  // message namespace
  "NotifyServiceState",  // message name
  3,  // number of fields
  sizeof(mmf_interfaces__msg__NotifyServiceState),
  mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_member_array,  // message members
  mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_init_function,  // function to initialize message memory (memory has to be allocated)
  mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_type_support_handle = {
  0,
  &mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mmf_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, NotifyServiceState)() {
  if (!mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_type_support_handle.typesupport_identifier) {
    mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mmf_interfaces__msg__NotifyServiceState__rosidl_typesupport_introspection_c__NotifyServiceState_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
