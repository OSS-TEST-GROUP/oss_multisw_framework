// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/DiagnosticInfoListRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/diagnostic_info_list_request__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const DiagnosticInfoListRequest & msg,
  std::ostream & out)
{
  (void)msg;
  out << "null";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DiagnosticInfoListRequest & msg,
  std::ostream & out, size_t indentation = 0)
{
  (void)msg;
  (void)indentation;
  out << "null\n";
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DiagnosticInfoListRequest & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::DiagnosticInfoListRequest & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::DiagnosticInfoListRequest & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::DiagnosticInfoListRequest>()
{
  return "mmf_interfaces::msg::DiagnosticInfoListRequest";
}

template<>
inline const char * name<mmf_interfaces::msg::DiagnosticInfoListRequest>()
{
  return "mmf_interfaces/msg/DiagnosticInfoListRequest";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::DiagnosticInfoListRequest>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::DiagnosticInfoListRequest>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<mmf_interfaces::msg::DiagnosticInfoListRequest>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_REQUEST__TRAITS_HPP_
