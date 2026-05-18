// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mmf_interfaces:msg/GetAppUpdateState.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mmf_interfaces/msg/detail/get_app_update_state__struct.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

namespace mmf_interfaces
{

namespace msg
{

namespace rosidl_typesupport_introspection_cpp
{

void GetAppUpdateState_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mmf_interfaces::msg::GetAppUpdateState(_init);
}

void GetAppUpdateState_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mmf_interfaces::msg::GetAppUpdateState *>(message_memory);
  typed_message->~GetAppUpdateState();
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember GetAppUpdateState_message_member_array[2] = {
  {
    "id",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::GetAppUpdateState, id),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "app_name",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::GetAppUpdateState, app_name),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers GetAppUpdateState_message_members = {
  "mmf_interfaces::msg",  // message namespace
  "GetAppUpdateState",  // message name
  2,  // number of fields
  sizeof(mmf_interfaces::msg::GetAppUpdateState),
  GetAppUpdateState_message_member_array,  // message members
  GetAppUpdateState_init_function,  // function to initialize message memory (memory has to be allocated)
  GetAppUpdateState_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t GetAppUpdateState_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &GetAppUpdateState_message_members,
  get_message_typesupport_handle_function,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace mmf_interfaces


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<mmf_interfaces::msg::GetAppUpdateState>()
{
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::GetAppUpdateState_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mmf_interfaces, msg, GetAppUpdateState)() {
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::GetAppUpdateState_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
