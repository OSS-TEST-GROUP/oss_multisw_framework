// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/StopServiceResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/stop_service_response__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const StopServiceResponse & msg,
  std::ostream & out)
{
  out << "{";
  // member: ret_code
  {
    out << "ret_code: ";
    rosidl_generator_traits::value_to_yaml(msg.ret_code, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const StopServiceResponse & msg,
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
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const StopServiceResponse & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::StopServiceResponse & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::StopServiceResponse & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::StopServiceResponse>()
{
  return "mmf_interfaces::msg::StopServiceResponse";
}

template<>
inline const char * name<mmf_interfaces::msg::StopServiceResponse>()
{
  return "mmf_interfaces/msg/StopServiceResponse";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::StopServiceResponse>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::StopServiceResponse>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<mmf_interfaces::msg::StopServiceResponse>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__TRAITS_HPP_
