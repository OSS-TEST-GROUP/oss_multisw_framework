// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mmf_interfaces:msg/GetServiceStateRequest.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mmf_interfaces/msg/detail/get_service_state_request__struct.hpp"
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

void GetServiceStateRequest_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mmf_interfaces::msg::GetServiceStateRequest(_init);
}

void GetServiceStateRequest_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mmf_interfaces::msg::GetServiceStateRequest *>(message_memory);
  typed_message->~GetServiceStateRequest();
}

size_t size_function__GetServiceStateRequest__service_list(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<std::string> *>(untyped_member);
  return member->size();
}

const void * get_const_function__GetServiceStateRequest__service_list(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<std::string> *>(untyped_member);
  return &member[index];
}

void * get_function__GetServiceStateRequest__service_list(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<std::string> *>(untyped_member);
  return &member[index];
}

void fetch_function__GetServiceStateRequest__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const std::string *>(
    get_const_function__GetServiceStateRequest__service_list(untyped_member, index));
  auto & value = *reinterpret_cast<std::string *>(untyped_value);
  value = item;
}

void assign_function__GetServiceStateRequest__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<std::string *>(
    get_function__GetServiceStateRequest__service_list(untyped_member, index));
  const auto & value = *reinterpret_cast<const std::string *>(untyped_value);
  item = value;
}

void resize_function__GetServiceStateRequest__service_list(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<std::string> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember GetServiceStateRequest_message_member_array[3] = {
  {
    "os_type",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::GetServiceStateRequest, os_type),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "service_list",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::GetServiceStateRequest, service_list),  // bytes offset in struct
    nullptr,  // default value
    size_function__GetServiceStateRequest__service_list,  // size() function pointer
    get_const_function__GetServiceStateRequest__service_list,  // get_const(index) function pointer
    get_function__GetServiceStateRequest__service_list,  // get(index) function pointer
    fetch_function__GetServiceStateRequest__service_list,  // fetch(index, &value) function pointer
    assign_function__GetServiceStateRequest__service_list,  // assign(index, value) function pointer
    resize_function__GetServiceStateRequest__service_list  // resize(index) function pointer
  },
  {
    "is_all_service",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::GetServiceStateRequest, is_all_service),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers GetServiceStateRequest_message_members = {
  "mmf_interfaces::msg",  // message namespace
  "GetServiceStateRequest",  // message name
  3,  // number of fields
  sizeof(mmf_interfaces::msg::GetServiceStateRequest),
  GetServiceStateRequest_message_member_array,  // message members
  GetServiceStateRequest_init_function,  // function to initialize message memory (memory has to be allocated)
  GetServiceStateRequest_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t GetServiceStateRequest_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &GetServiceStateRequest_message_members,
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
get_message_type_support_handle<mmf_interfaces::msg::GetServiceStateRequest>()
{
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::GetServiceStateRequest_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mmf_interfaces, msg, GetServiceStateRequest)() {
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::GetServiceStateRequest_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
