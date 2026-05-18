// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/SetOtaClientRegistration.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/set_ota_client_registration__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const SetOtaClientRegistration & msg,
  std::ostream & out)
{
  out << "{";
  // member: client_name
  {
    out << "client_name: ";
    rosidl_generator_traits::value_to_yaml(msg.client_name, out);
    out << ", ";
  }

  // member: app_list
  {
    if (msg.app_list.size() == 0) {
      out << "app_list: []";
    } else {
      out << "app_list: [";
      size_t pending_items = msg.app_list.size();
      for (auto item : msg.app_list) {
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
  const SetOtaClientRegistration & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: client_name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "client_name: ";
    rosidl_generator_traits::value_to_yaml(msg.client_name, out);
    out << "\n";
  }

  // member: app_list
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.app_list.size() == 0) {
      out << "app_list: []\n";
    } else {
      out << "app_list:\n";
      for (auto item : msg.app_list) {
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

inline std::string to_yaml(const SetOtaClientRegistration & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::SetOtaClientRegistration & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::SetOtaClientRegistration & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::SetOtaClientRegistration>()
{
  return "mmf_interfaces::msg::SetOtaClientRegistration";
}

template<>
inline const char * name<mmf_interfaces::msg::SetOtaClientRegistration>()
{
  return "mmf_interfaces/msg/SetOtaClientRegistration";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::SetOtaClientRegistration>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::SetOtaClientRegistration>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::SetOtaClientRegistration>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__TRAITS_HPP_
