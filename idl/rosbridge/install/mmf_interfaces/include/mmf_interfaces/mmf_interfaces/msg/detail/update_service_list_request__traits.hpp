// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/UpdateServiceListRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/update_service_list_request__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'service_list'
#include "mmf_interfaces/msg/detail/lifecycle_service__traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const UpdateServiceListRequest & msg,
  std::ostream & out)
{
  out << "{";
  // member: service_list
  {
    if (msg.service_list.size() == 0) {
      out << "service_list: []";
    } else {
      out << "service_list: [";
      size_t pending_items = msg.service_list.size();
      for (auto item : msg.service_list) {
        to_flow_style_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const UpdateServiceListRequest & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: service_list
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.service_list.size() == 0) {
      out << "service_list: []\n";
    } else {
      out << "service_list:\n";
      for (auto item : msg.service_list) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "-\n";
        to_block_style_yaml(item, out, indentation + 2);
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const UpdateServiceListRequest & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace mmf_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use mmf_interfaces::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const mmf_interfaces::msg::UpdateServiceListRequest & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::UpdateServiceListRequest & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::UpdateServiceListRequest>()
{
  return "mmf_interfaces::msg::UpdateServiceListRequest";
}

template<>
inline const char * name<mmf_interfaces::msg::UpdateServiceListRequest>()
{
  return "mmf_interfaces/msg/UpdateServiceListRequest";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::UpdateServiceListRequest>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::UpdateServiceListRequest>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::UpdateServiceListRequest>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__TRAITS_HPP_
