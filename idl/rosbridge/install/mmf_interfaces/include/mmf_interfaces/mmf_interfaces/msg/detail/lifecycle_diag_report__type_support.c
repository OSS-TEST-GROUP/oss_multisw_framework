// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mmf_interfaces:msg/LifecycleDiagReport.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mmf_interfaces/msg/detail/lifecycle_diag_report__rosidl_typesupport_introspection_c.h"
#include "mmf_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mmf_interfaces/msg/detail/lifecycle_diag_report__functions.h"
#include "mmf_interfaces/msg/detail/lifecycle_diag_report__struct.h"


// Include directives for member types
// Member `service_list`
#include "mmf_interfaces/msg/service_status.h"
// Member `service_list`
#include "mmf_interfaces/msg/detail/service_status__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mmf_interfaces__msg__LifecycleDiagReport__init(message_memory);
}

void mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_fini_function(void * message_memory)
{
  mmf_interfaces__msg__LifecycleDiagReport__fini(message_memory);
}

size_t mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__size_function__LifecycleDiagReport__service_list(
  const void * untyped_member)
{
  const mmf_interfaces__msg__ServiceStatus__Sequence * member =
    (const mmf_interfaces__msg__ServiceStatus__Sequence *)(untyped_member);
  return member->size;
}

const void * mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_const_function__LifecycleDiagReport__service_list(
  const void * untyped_member, size_t index)
{
  const mmf_interfaces__msg__ServiceStatus__Sequence * member =
    (const mmf_interfaces__msg__ServiceStatus__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_function__LifecycleDiagReport__service_list(
  void * untyped_member, size_t index)
{
  mmf_interfaces__msg__ServiceStatus__Sequence * member =
    (mmf_interfaces__msg__ServiceStatus__Sequence *)(untyped_member);
  return &member->data[index];
}

void mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__fetch_function__LifecycleDiagReport__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const mmf_interfaces__msg__ServiceStatus * item =
    ((const mmf_interfaces__msg__ServiceStatus *)
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_const_function__LifecycleDiagReport__service_list(untyped_member, index));
  mmf_interfaces__msg__ServiceStatus * value =
    (mmf_interfaces__msg__ServiceStatus *)(untyped_value);
  *value = *item;
}

void mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__assign_function__LifecycleDiagReport__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  mmf_interfaces__msg__ServiceStatus * item =
    ((mmf_interfaces__msg__ServiceStatus *)
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_function__LifecycleDiagReport__service_list(untyped_member, index));
  const mmf_interfaces__msg__ServiceStatus * value =
    (const mmf_interfaces__msg__ServiceStatus *)(untyped_value);
  *item = *value;
}

bool mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__resize_function__LifecycleDiagReport__service_list(
  void * untyped_member, size_t size)
{
  mmf_interfaces__msg__ServiceStatus__Sequence * member =
    (mmf_interfaces__msg__ServiceStatus__Sequence *)(untyped_member);
  mmf_interfaces__msg__ServiceStatus__Sequence__fini(member);
  return mmf_interfaces__msg__ServiceStatus__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_member_array[4] = {
  {
    "is_started",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__LifecycleDiagReport, is_started),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "running_service_num",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__LifecycleDiagReport, running_service_num),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "total_services_num",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__LifecycleDiagReport, total_services_num),  // bytes offset in struct
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
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__LifecycleDiagReport, service_list),  // bytes offset in struct
    NULL,  // default value
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__size_function__LifecycleDiagReport__service_list,  // size() function pointer
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_const_function__LifecycleDiagReport__service_list,  // get_const(index) function pointer
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__get_function__LifecycleDiagReport__service_list,  // get(index) function pointer
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__fetch_function__LifecycleDiagReport__service_list,  // fetch(index, &value) function pointer
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__assign_function__LifecycleDiagReport__service_list,  // assign(index, value) function pointer
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__resize_function__LifecycleDiagReport__service_list  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_members = {
  "mmf_interfaces__msg",  // message namespace
  "LifecycleDiagReport",  // message name
  4,  // number of fields
  sizeof(mmf_interfaces__msg__LifecycleDiagReport),
  mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_member_array,  // message members
  mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_init_function,  // function to initialize message memory (memory has to be allocated)
  mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_type_support_handle = {
  0,
  &mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mmf_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, LifecycleDiagReport)() {
  mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_member_array[3].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, ServiceStatus)();
  if (!mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_type_support_handle.typesupport_identifier) {
    mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mmf_interfaces__msg__LifecycleDiagReport__rosidl_typesupport_introspection_c__LifecycleDiagReport_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
