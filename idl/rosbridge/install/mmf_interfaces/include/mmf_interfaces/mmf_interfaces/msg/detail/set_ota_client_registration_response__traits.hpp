// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/SetOtaClientRegistrationResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/set_ota_client_registration_response__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const SetOtaClientRegistrationResponse & msg,
  std::ostream & out)
{
  out << "{";
  // member: client_id
  {
    out << "client_id: ";
    rosidl_generator_traits::value_to_yaml(msg.client_id, out);
    out << ", ";
  }

  // member: ret_code
  {
    out << "ret_code: ";
    rosidl_generator_traits::value_to_yaml(msg.ret_code, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const SetOtaClientRegistrationResponse & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: client_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "client_id: ";
    rosidl_generator_traits::value_to_yaml(msg.client_id, out);
    out << "\n";
  }

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

inline std::string to_yaml(const SetOtaClientRegistrationResponse & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::SetOtaClientRegistrationResponse & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::SetOtaClientRegistrationResponse & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::SetOtaClientRegistrationResponse>()
{
  return "mmf_interfaces::msg::SetOtaClientRegistrationResponse";
}

template<>
inline const char * name<mmf_interfaces::msg::SetOtaClientRegistrationResponse>()
{
  return "mmf_interfaces/msg/SetOtaClientRegistrationResponse";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::SetOtaClientRegistrationResponse>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::SetOtaClientRegistrationResponse>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<mmf_interfaces::msg::SetOtaClientRegistrationResponse>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__TRAITS_HPP_
