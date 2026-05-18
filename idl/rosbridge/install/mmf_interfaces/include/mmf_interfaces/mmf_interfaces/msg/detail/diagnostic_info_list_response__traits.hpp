// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/DiagnosticInfoListResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/diagnostic_info_list_response__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const DiagnosticInfoListResponse & msg,
  std::ostream & out)
{
  out << "{";
  // member: ret_code
  {
    out << "ret_code: ";
    rosidl_generator_traits::value_to_yaml(msg.ret_code, out);
    out << ", ";
  }

  // member: diag_codes
  {
    if (msg.diag_codes.size() == 0) {
      out << "diag_codes: []";
    } else {
      out << "diag_codes: [";
      size_t pending_items = msg.diag_codes.size();
      for (auto item : msg.diag_codes) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: diag_names
  {
    if (msg.diag_names.size() == 0) {
      out << "diag_names: []";
    } else {
      out << "diag_names: [";
      size_t pending_items = msg.diag_names.size();
      for (auto item : msg.diag_names) {
        rosidl_generator_traits::value_to_yaml(item, out);
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
  const DiagnosticInfoListResponse & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: ret_code
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "ret_code: ";
    rosidl_generator_traits::value_to_yaml(msg.ret_code, out);
    out << "\n";
  }

  // member: diag_codes
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.diag_codes.size() == 0) {
      out << "diag_codes: []\n";
    } else {
      out << "diag_codes:\n";
      for (auto item : msg.diag_codes) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: diag_names
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.diag_names.size() == 0) {
      out << "diag_names: []\n";
    } else {
      out << "diag_names:\n";
      for (auto item : msg.diag_names) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DiagnosticInfoListResponse & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::DiagnosticInfoListResponse & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::DiagnosticInfoListResponse & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::DiagnosticInfoListResponse>()
{
  return "mmf_interfaces::msg::DiagnosticInfoListResponse";
}

template<>
inline const char * name<mmf_interfaces::msg::DiagnosticInfoListResponse>()
{
  return "mmf_interfaces/msg/DiagnosticInfoListResponse";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::DiagnosticInfoListResponse>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::DiagnosticInfoListResponse>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::DiagnosticInfoListResponse>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__TRAITS_HPP_
