// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/NotifyAppUpdateStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/notify_app_update_status__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_NotifyAppUpdateStatus_status
{
public:
  explicit Init_NotifyAppUpdateStatus_status(::mmf_interfaces::msg::NotifyAppUpdateStatus & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::NotifyAppUpdateStatus status(::mmf_interfaces::msg::NotifyAppUpdateStatus::_status_type arg)
  {
    msg_.status = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyAppUpdateStatus msg_;
};

class Init_NotifyAppUpdateStatus_app_name
{
public:
  explicit Init_NotifyAppUpdateStatus_app_name(::mmf_interfaces::msg::NotifyAppUpdateStatus & msg)
  : msg_(msg)
  {}
  Init_NotifyAppUpdateStatus_status app_name(::mmf_interfaces::msg::NotifyAppUpdateStatus::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return Init_NotifyAppUpdateStatus_status(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyAppUpdateStatus msg_;
};

class Init_NotifyAppUpdateStatus_client_id
{
public:
  Init_NotifyAppUpdateStatus_client_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_NotifyAppUpdateStatus_app_name client_id(::mmf_interfaces::msg::NotifyAppUpdateStatus::_client_id_type arg)
  {
    msg_.client_id = std::move(arg);
    return Init_NotifyAppUpdateStatus_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyAppUpdateStatus msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::NotifyAppUpdateStatus>()
{
  return mmf_interfaces::msg::builder::Init_NotifyAppUpdateStatus_client_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__BUILDER_HPP_
