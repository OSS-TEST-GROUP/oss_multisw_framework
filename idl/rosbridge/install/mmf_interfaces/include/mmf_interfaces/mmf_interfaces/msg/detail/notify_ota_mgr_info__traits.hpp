// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/NotifyOtaMgrInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/notify_ota_mgr_info__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const NotifyOtaMgrInfo & msg,
  std::ostream & out)
{
  out << "{";
  // member: err_code
  {
    out << "err_code: ";
    rosidl_generator_traits::value_to_yaml(msg.err_code, out);
    out << ", ";
  }

  // member: err_msg
  {
    out << "err_msg: ";
    rosidl_generator_traits::value_to_yaml(msg.err_msg, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const NotifyOtaMgrInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: err_code
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "err_code: ";
    rosidl_generator_traits::value_to_yaml(msg.err_code, out);
    out << "\n";
  }

  // member: err_msg
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "err_msg: ";
    rosidl_generator_traits::value_to_yaml(msg.err_msg, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const NotifyOtaMgrInfo & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::NotifyOtaMgrInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::NotifyOtaMgrInfo & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::NotifyOtaMgrInfo>()
{
  return "mmf_interfaces::msg::NotifyOtaMgrInfo";
}

template<>
inline const char * name<mmf_interfaces::msg::NotifyOtaMgrInfo>()
{
  return "mmf_interfaces/msg/NotifyOtaMgrInfo";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::NotifyOtaMgrInfo>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::NotifyOtaMgrInfo>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::NotifyOtaMgrInfo>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__TRAITS_HPP_
