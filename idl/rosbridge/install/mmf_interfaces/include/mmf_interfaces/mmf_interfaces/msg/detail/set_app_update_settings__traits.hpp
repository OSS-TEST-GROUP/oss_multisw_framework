// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/SetAppUpdateSettings.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/set_app_update_settings__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const SetAppUpdateSettings & msg,
  std::ostream & out)
{
  out << "{";
  // member: id
  {
    out << "id: ";
    rosidl_generator_traits::value_to_yaml(msg.id, out);
    out << ", ";
  }

  // member: app_name
  {
    out << "app_name: ";
    rosidl_generator_traits::value_to_yaml(msg.app_name, out);
    out << ", ";
  }

  // member: app_settings
  {
    out << "app_settings: ";
    rosidl_generator_traits::value_to_yaml(msg.app_settings, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const SetAppUpdateSettings & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "id: ";
    rosidl_generator_traits::value_to_yaml(msg.id, out);
    out << "\n";
  }

  // member: app_name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "app_name: ";
    rosidl_generator_traits::value_to_yaml(msg.app_name, out);
    out << "\n";
  }

  // member: app_settings
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "app_settings: ";
    rosidl_generator_traits::value_to_yaml(msg.app_settings, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const SetAppUpdateSettings & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::SetAppUpdateSettings & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::SetAppUpdateSettings & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::SetAppUpdateSettings>()
{
  return "mmf_interfaces::msg::SetAppUpdateSettings";
}

template<>
inline const char * name<mmf_interfaces::msg::SetAppUpdateSettings>()
{
  return "mmf_interfaces/msg/SetAppUpdateSettings";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::SetAppUpdateSettings>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::SetAppUpdateSettings>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::SetAppUpdateSettings>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__TRAITS_HPP_
