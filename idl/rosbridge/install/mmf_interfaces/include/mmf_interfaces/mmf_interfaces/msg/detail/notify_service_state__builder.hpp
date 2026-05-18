// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/NotifyServiceState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/notify_service_state__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_NotifyServiceState_service_state_list
{
public:
  explicit Init_NotifyServiceState_service_state_list(::mmf_interfaces::msg::NotifyServiceState & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::NotifyServiceState service_state_list(::mmf_interfaces::msg::NotifyServiceState::_service_state_list_type arg)
  {
    msg_.service_state_list = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyServiceState msg_;
};

class Init_NotifyServiceState_service_list
{
public:
  explicit Init_NotifyServiceState_service_list(::mmf_interfaces::msg::NotifyServiceState & msg)
  : msg_(msg)
  {}
  Init_NotifyServiceState_service_state_list service_list(::mmf_interfaces::msg::NotifyServiceState::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return Init_NotifyServiceState_service_state_list(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyServiceState msg_;
};

class Init_NotifyServiceState_os_type
{
public:
  Init_NotifyServiceState_os_type()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_NotifyServiceState_service_list os_type(::mmf_interfaces::msg::NotifyServiceState::_os_type_type arg)
  {
    msg_.os_type = std::move(arg);
    return Init_NotifyServiceState_service_list(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyServiceState msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::NotifyServiceState>()
{
  return mmf_interfaces::msg::builder::Init_NotifyServiceState_os_type();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__BUILDER_HPP_
