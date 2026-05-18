// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/ServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/service_status__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const ServiceStatus & msg,
  std::ostream & out)
{
  out << "{";
  // member: name
  {
    out << "name: ";
    rosidl_generator_traits::value_to_yaml(msg.name, out);
    out << ", ";
  }

  // member: service_type
  {
    out << "service_type: ";
    rosidl_generator_traits::value_to_yaml(msg.service_type, out);
    out << ", ";
  }

  // member: service_state
  {
    out << "service_state: ";
    rosidl_generator_traits::value_to_yaml(msg.service_state, out);
    out << ", ";
  }

  // member: is_enabled
  {
    out << "is_enabled: ";
    rosidl_generator_traits::value_to_yaml(msg.is_enabled, out);
    out << ", ";
  }

  // member: recovered_count
  {
    out << "recovered_count: ";
    rosidl_generator_traits::value_to_yaml(msg.recovered_count, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const ServiceStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "name: ";
    rosidl_generator_traits::value_to_yaml(msg.name, out);
    out << "\n";
  }

  // member: service_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "service_type: ";
    rosidl_generator_traits::value_to_yaml(msg.service_type, out);
    out << "\n";
  }

  // member: service_state
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "service_state: ";
    rosidl_generator_traits::value_to_yaml(msg.service_state, out);
    out << "\n";
  }

  // member: is_enabled
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "is_enabled: ";
    rosidl_generator_traits::value_to_yaml(msg.is_enabled, out);
    out << "\n";
  }

  // member: recovered_count
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "recovered_count: ";
    rosidl_generator_traits::value_to_yaml(msg.recovered_count, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const ServiceStatus & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::ServiceStatus & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::ServiceStatus & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::ServiceStatus>()
{
  return "mmf_interfaces::msg::ServiceStatus";
}

template<>
inline const char * name<mmf_interfaces::msg::ServiceStatus>()
{
  return "mmf_interfaces/msg/ServiceStatus";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::ServiceStatus>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::ServiceStatus>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::ServiceStatus>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__TRAITS_HPP_
