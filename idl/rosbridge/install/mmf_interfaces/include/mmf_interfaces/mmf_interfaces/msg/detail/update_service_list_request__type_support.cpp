// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mmf_interfaces:msg/UpdateServiceListRequest.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mmf_interfaces/msg/detail/update_service_list_request__struct.hpp"
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

void UpdateServiceListRequest_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mmf_interfaces::msg::UpdateServiceListRequest(_init);
}

void UpdateServiceListRequest_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mmf_interfaces::msg::UpdateServiceListRequest *>(message_memory);
  typed_message->~UpdateServiceListRequest();
}

size_t size_function__UpdateServiceListRequest__service_list(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<mmf_interfaces::msg::LifecycleService> *>(untyped_member);
  return member->size();
}

const void * get_const_function__UpdateServiceListRequest__service_list(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<mmf_interfaces::msg::LifecycleService> *>(untyped_member);
  return &member[index];
}

void * get_function__UpdateServiceListRequest__service_list(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<mmf_interfaces::msg::LifecycleService> *>(untyped_member);
  return &member[index];
}

void fetch_function__UpdateServiceListRequest__service_list(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const mmf_interfaces::msg::LifecycleService *>(
    get_const_function__UpdateServiceListRequest__service_list(untyped_member, index));
  auto & value = *reinterpret_cast<mmf_interfaces::msg::LifecycleService *>(untyped_value);
  value = item;
}

void assign_function__UpdateServiceListRequest__service_list(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<mmf_interfaces::msg::LifecycleService *>(
    get_function__UpdateServiceListRequest__service_list(untyped_member, index));
  const auto & value = *reinterpret_cast<const mmf_interfaces::msg::LifecycleService *>(untyped_value);
  item = value;
}

void resize_function__UpdateServiceListRequest__service_list(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<mmf_interfaces::msg::LifecycleService> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember UpdateServiceListRequest_message_member_array[1] = {
  {
    "service_list",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<mmf_interfaces::msg::LifecycleService>(),  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mmf_interfaces::msg::UpdateServiceListRequest, service_list),  // bytes offset in struct
    nullptr,  // default value
    size_function__UpdateServiceListRequest__service_list,  // size() function pointer
    get_const_function__UpdateServiceListRequest__service_list,  // get_const(index) function pointer
    get_function__UpdateServiceListRequest__service_list,  // get(index) function pointer
    fetch_function__UpdateServiceListRequest__service_list,  // fetch(index, &value) function pointer
    assign_function__UpdateServiceListRequest__service_list,  // assign(index, value) function pointer
    resize_function__UpdateServiceListRequest__service_list  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers UpdateServiceListRequest_message_members = {
  "mmf_interfaces::msg",  // message namespace
  "UpdateServiceListRequest",  // message name
  1,  // number of fields
  sizeof(mmf_interfaces::msg::UpdateServiceListRequest),
  UpdateServiceListRequest_message_member_array,  // message members
  UpdateServiceListRequest_init_function,  // function to initialize message memory (memory has to be allocated)
  UpdateServiceListRequest_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t UpdateServiceListRequest_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &UpdateServiceListRequest_message_members,
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
get_message_type_support_handle<mmf_interfaces::msg::UpdateServiceListRequest>()
{
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::UpdateServiceListRequest_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mmf_interfaces, msg, UpdateServiceListRequest)() {
  return &::mmf_interfaces::msg::rosidl_typesupport_introspection_cpp::UpdateServiceListRequest_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
