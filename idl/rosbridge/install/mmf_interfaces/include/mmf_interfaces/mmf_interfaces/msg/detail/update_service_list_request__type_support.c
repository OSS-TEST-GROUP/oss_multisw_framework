// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mmf_interfaces:msg/UpdateServiceListRequest.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mmf_interfaces/msg/detail/update_service_list_request__rosidl_typesupport_introspection_c.h"
#include "mmf_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mmf_interfaces/msg/detail/update_service_list_request__functions.h"
#include "mmf_interfaces/msg/detail/update_service_list_request__struct.h"


// Include directives for member types
// Member `service_list`
#include "mmf_interfaces/msg/lifecycle_service.h"
// Member `service_list`
#include "mmf_interfaces/msg/detail/lifecycle_service__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mmf_interfaces__msg__UpdateServiceListRequest__init(message_memory);
}

void mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_fini_function(void * message_memory)
{
  mmf_interfaces__msg__UpdateServiceListRequest__fini(message_memory);
}

size_t mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__size_function__UpdateServiceListRequest__service_list(
  const void * untyped_member)
{
  const mmf_interfaces__msg__LifecycleService__Sequence * member =
    (const mmf_interfaces__msg__LifecycleService__Sequence *)(untyped_member);
  return member->size;
}

const void * mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_const_function__UpdateServiceListRequest__service_list(
  const void * untyped_member, size_t index)
{
  const mmf_interfaces__msg__LifecycleService__Sequence * member =
    (const mmf_interfaces__msg__LifecycleService__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_function__UpdateServiceListRequest__service_list(
  void * untyped_member, size_t index)
{
  mmf_interfaces__msg__LifecycleService__Sequence * member =
    (mmf_interfaces__msg__LifecycleService__Sequence *)(untyped_member);
  return &member->data[index];
}

void mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__fetch_function__UpdateServiceListRequest__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const mmf_interfaces__msg__LifecycleService * item =
    ((const mmf_interfaces__msg__LifecycleService *)
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_const_function__UpdateServiceListRequest__service_list(untyped_member, index));
  mmf_interfaces__msg__LifecycleService * value =
    (mmf_interfaces__msg__LifecycleService *)(untyped_value);
  *value = *item;
}

void mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__assign_function__UpdateServiceListRequest__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  mmf_interfaces__msg__LifecycleService * item =
    ((mmf_interfaces__msg__LifecycleService *)
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_function__UpdateServiceListRequest__service_list(untyped_member, index));
  const mmf_interfaces__msg__LifecycleService * value =
    (const mmf_interfaces__msg__LifecycleService *)(untyped_value);
  *item = *value;
}

bool mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__resize_function__UpdateServiceListRequest__service_list(
  void * untyped_member, size_t size)
{
  mmf_interfaces__msg__LifecycleService__Sequence * member =
    (mmf_interfaces__msg__LifecycleService__Sequence *)(untyped_member);
  mmf_interfaces__msg__LifecycleService__Sequence__fini(member);
  return mmf_interfaces__msg__LifecycleService__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_member_array[1] = {
  {
    "service_list",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces__msg__UpdateServiceListRequest, service_list),  // bytes offset in struct
    NULL,  // default value
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__size_function__UpdateServiceListRequest__service_list,  // size() function pointer
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_const_function__UpdateServiceListRequest__service_list,  // get_const(index) function pointer
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__get_function__UpdateServiceListRequest__service_list,  // get(index) function pointer
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__fetch_function__UpdateServiceListRequest__service_list,  // fetch(index, &value) function pointer
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__assign_function__UpdateServiceListRequest__service_list,  // assign(index, value) function pointer
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__resize_function__UpdateServiceListRequest__service_list  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_members = {
  "mmf_interfaces__msg",  // message namespace
  "UpdateServiceListRequest",  // message name
  1,  // number of fields
  sizeof(mmf_interfaces__msg__UpdateServiceListRequest),
  mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_member_array,  // message members
  mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_init_function,  // function to initialize message memory (memory has to be allocated)
  mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_type_support_handle = {
  0,
  &mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mmf_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, UpdateServiceListRequest)() {
  mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_member_array[0].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mmf_interfaces, msg, LifecycleService)();
  if (!mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_type_support_handle.typesupport_identifier) {
    mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mmf_interfaces__msg__UpdateServiceListRequest__rosidl_typesupport_introspection_c__UpdateServiceListRequest_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
