// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/DiagnosticNotification.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/diagnostic_notification__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const DiagnosticNotification & msg,
  std::ostream & out)
{
  out << "{";
  // member: diag_code
  {
    out << "diag_code: ";
    rosidl_generator_traits::value_to_yaml(msg.diag_code, out);
    out << ", ";
  }

  // member: diag_name
  {
    out << "diag_name: ";
    rosidl_generator_traits::value_to_yaml(msg.diag_name, out);
    out << ", ";
  }

  // member: status
  {
    out << "status: ";
    rosidl_generator_traits::value_to_yaml(msg.status, out);
    out << ", ";
  }

  // member: message
  {
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
    out << ", ";
  }

  // member: source_message_name
  {
    out << "source_message_name: ";
    rosidl_generator_traits::value_to_yaml(msg.source_message_name, out);
    out << ", ";
  }

  // member: raw_data
  {
    out << "raw_data: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_data, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DiagnosticNotification & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: diag_code
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "diag_code: ";
    rosidl_generator_traits::value_to_yaml(msg.diag_code, out);
    out << "\n";
  }

  // member: diag_name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "diag_name: ";
    rosidl_generator_traits::value_to_yaml(msg.diag_name, out);
    out << "\n";
  }

  // member: status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "status: ";
    rosidl_generator_traits::value_to_yaml(msg.status, out);
    out << "\n";
  }

  // member: message
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
    out << "\n";
  }

  // member: source_message_name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "source_message_name: ";
    rosidl_generator_traits::value_to_yaml(msg.source_message_name, out);
    out << "\n";
  }

  // member: raw_data
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "raw_data: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_data, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DiagnosticNotification & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::DiagnosticNotification & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::DiagnosticNotification & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::DiagnosticNotification>()
{
  return "mmf_interfaces::msg::DiagnosticNotification";
}

template<>
inline const char * name<mmf_interfaces::msg::DiagnosticNotification>()
{
  return "mmf_interfaces/msg/DiagnosticNotification";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::DiagnosticNotification>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::DiagnosticNotification>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::DiagnosticNotification>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__TRAITS_HPP_
