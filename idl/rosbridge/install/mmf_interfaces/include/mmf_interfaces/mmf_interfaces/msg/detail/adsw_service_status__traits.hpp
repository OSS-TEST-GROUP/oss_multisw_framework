// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/AdswServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/adsw_service_status__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const AdswServiceStatus & msg,
  std::ostream & out)
{
  out << "{";
  // member: status_code
  {
    out << "status_code: ";
    rosidl_generator_traits::value_to_yaml(msg.status_code, out);
    out << ", ";
  }

  // member: status_msg
  {
    out << "status_msg: ";
    rosidl_generator_traits::value_to_yaml(msg.status_msg, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const AdswServiceStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: status_code
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "status_code: ";
    rosidl_generator_traits::value_to_yaml(msg.status_code, out);
    out << "\n";
  }

  // member: status_msg
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "status_msg: ";
    rosidl_generator_traits::value_to_yaml(msg.status_msg, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const AdswServiceStatus & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::AdswServiceStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::AdswServiceStatus & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::AdswServiceStatus>()
{
  return "mmf_interfaces::msg::AdswServiceStatus";
}

template<>
inline const char * name<mmf_interfaces::msg::AdswServiceStatus>()
{
  return "mmf_interfaces/msg/AdswServiceStatus";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::AdswServiceStatus>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::AdswServiceStatus>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::AdswServiceStatus>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__TRAITS_HPP_
