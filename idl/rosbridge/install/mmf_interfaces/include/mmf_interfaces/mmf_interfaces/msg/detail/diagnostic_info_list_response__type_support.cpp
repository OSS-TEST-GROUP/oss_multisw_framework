// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mmf_interfaces:msg/DiagnosticInfoListResponse.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mmf_interfaces/msg/detail/diagnostic_info_list_response__struct.hpp"
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

void DiagnosticInfoListResponse_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mmf_interfaces::msg::DiagnosticInfoListResponse(_init);
}

void DiagnosticInfoListResponse_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mmf_interfaces::msg::DiagnosticInfoListResponse *>(message_memory);
  typed_message->~DiagnosticInfoListResponse();
}

size_t size_function__DiagnosticInfoListResponse__diag_codes(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<int32_t> *>(untyped_member);
  return member->size();
}

const void * get_const_function__DiagnosticInfoListResponse__diag_codes(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<int32_t> *>(untyped_member);
  return &member[index];
}

void * get_function__DiagnosticInfoListResponse__diag_codes(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<int32_t> *>(untyped_member);
  return &member[index];
}

void fetch_function__DiagnosticInfoListResponse__diag_codes(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int32_t *>(
    get_const_function__DiagnosticInfoListResponse__diag_codes(untyped_member, index));
  auto & value = *reinterpret_cast<int32_t *>(untyped_value);
  value = item;
}

void assign_function__DiagnosticInfoListResponse__diag_codes(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int32_t *>(
    get_function__DiagnosticInfoListResponse__diag_codes(untyped_member, index));
  const auto & value = *reinterpret_cast<const int32_t *>(untyped_value);
  item = value;
}

void resize_function__DiagnosticInfoListResponse__diag_codes(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<int32_t> *>(untyped_member);
  member->resize(size);
}

size_t size_function__DiagnosticInfoListResponse__diag_names(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<std::string> *>(untyped_member);
  return member->size();
}

const void * get_const_function__DiagnosticInfoListResponse__diag_names(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<std::string> *>(untyped_member);
  return &member[index];
}

void * get_function__DiagnosticInfoListResponse__diag_names(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<std::string> *>(untyped_member);
  return &member[index];
}

void fetch_function__DiagnosticInfoListResponse__diag_names(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const std::string *>(
    get_const_function__DiagnosticInfoListResponse__diag_names(untyped_member, index));
  auto & value = *reinterpret_cast<std::string *>(untyped_value);
  value = item;
}

void assign_function__DiagnosticInfoListResponse__diag_names(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<std::string *>(
    get_function__DiagnosticInfoListResponse__diag_names(untyped_member, index));
  const auto & value = *reinterpret_cast<const std::string *>(untyped_value);
  item = value;
}

void resize_function__DiagnosticInfoListResponse__diag_names(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<std::string> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember DiagnosticInfoListResponse_message_member_array[3] = {
  {
    "ret_code",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::DiagnosticInfoListResponse, ret_code),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "diag_codes",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::DiagnosticInfoListResponse, diag_codes),  // bytes offset in struct
    nullptr,  // default value
    size_function__DiagnosticInfoListResponse__diag_codes,  // size() function pointer
    get_const_function__DiagnosticInfoListResponse__diag_codes,  // get_const(index) function pointer
    get_function__DiagnosticInfoListResponse__diag_codes,  // get(index) function pointer
    fetch_function__DiagnosticInfoListResponse__diag_codes,  // fetch(index, &value) function pointer
    assign_function__DiagnosticInfoListResponse__diag_codes,  // assign(index, value) function pointer
    resize_function__DiagnosticInfoListResponse__diag_codes  // resize(index) function pointer
  },
  {
    "diag_names",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::DiagnosticInfoListResponse, diag_names),  // bytes offset in struct
    nullptr,  // default value
    size_function__DiagnosticInfoListResponse__diag_names,  // size() function pointer
    get_const_function__DiagnosticInfoListResponse__diag_names,  // get_const(index) function pointer
    get_function__DiagnosticInfoListResponse__diag_names,  // get(index) function pointer
    fetch_function__DiagnosticInfoListResponse__diag_names,  // fetch(index, &value) function pointer
    assign_function__DiagnosticInfoListResponse__diag_names,  // assign(index, value) function pointer
    resize_function__DiagnosticInfoListResponse__diag_names  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers DiagnosticInfoListResponse_message_members = {
  "mmf_interfaces::msg",  // message namespace
  "DiagnosticInfoListResponse",  // message name
  3,  // number of fields
  sizeof(mmf_interfaces::msg::DiagnosticInfoListResponse),
  DiagnosticInfoListResponse_message_member_array,  // message members
  DiagnosticInfoListResponse_init_function,  // function to initialize message memory (memory has to be allocated)
  DiagnosticInfoListResponse_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t DiagnosticInfoListResponse_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &DiagnosticInfoListResponse_message_members,
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
get_message_type_support_handle<mmf_interfaces::msg::DiagnosticInfoListResponse>()
{
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::DiagnosticInfoListResponse_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mmf_interfaces, msg, DiagnosticInfoListResponse)() {
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::DiagnosticInfoListResponse_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
