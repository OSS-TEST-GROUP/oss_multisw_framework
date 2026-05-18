// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mmf_interfaces:msg/NotifyServiceState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__TRAITS_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mmf_interfaces/msg/detail/notify_service_state__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mmf_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const NotifyServiceState & msg,
  std::ostream & out)
{
  out << "{";
  // member: os_type
  {
    out << "os_type: ";
    rosidl_generator_traits::value_to_yaml(msg.os_type, out);
    out << ", ";
  }

  // member: service_list
  {
    if (msg.service_list.size() == 0) {
      out << "service_list: []";
    } else {
      out << "service_list: [";
      size_t pending_items = msg.service_list.size();
      for (auto item : msg.service_list) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: service_state_list
  {
    if (msg.service_state_list.size() == 0) {
      out << "service_state_list: []";
    } else {
      out << "service_state_list: [";
      size_t pending_items = msg.service_state_list.size();
      for (auto item : msg.service_state_list) {
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
  const NotifyServiceState & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: os_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "os_type: ";
    rosidl_generator_traits::value_to_yaml(msg.os_type, out);
    out << "\n";
  }

  // member: service_list
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.service_list.size() == 0) {
      out << "service_list: []\n";
    } else {
      out << "service_list:\n";
      for (auto item : msg.service_list) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: service_state_list
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.service_state_list.size() == 0) {
      out << "service_state_list: []\n";
    } else {
      out << "service_state_list:\n";
      for (auto item : msg.service_state_list) {
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

inline std::string to_yaml(const NotifyServiceState & msg, bool use_flow_style = false)
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
  const mmf_interfaces::msg::NotifyServiceState & msg,
  std::ostream & out, size_t indentation = 0)
{
  mmf_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mmf_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const mmf_interfaces::msg::NotifyServiceState & msg)
{
  return mmf_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mmf_interfaces::msg::NotifyServiceState>()
{
  return "mmf_interfaces::msg::NotifyServiceState";
}

template<>
inline const char * name<mmf_interfaces::msg::NotifyServiceState>()
{
  return "mmf_interfaces/msg/NotifyServiceState";
}

template<>
struct has_fixed_size<mmf_interfaces::msg::NotifyServiceState>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mmf_interfaces::msg::NotifyServiceState>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mmf_interfaces::msg::NotifyServiceState>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__TRAITS_HPP_
