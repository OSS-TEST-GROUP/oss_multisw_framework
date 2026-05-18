// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetAppUpdateSettings.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_app_update_settings__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetAppUpdateSettings_app_name
{
public:
  explicit Init_GetAppUpdateSettings_app_name(::mmf_interfaces::msg::GetAppUpdateSettings & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetAppUpdateSettings app_name(::mmf_interfaces::msg::GetAppUpdateSettings::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettings msg_;
};

class Init_GetAppUpdateSettings_id
{
public:
  Init_GetAppUpdateSettings_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetAppUpdateSettings_app_name id(::mmf_interfaces::msg::GetAppUpdateSettings::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetAppUpdateSettings_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettings msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetAppUpdateSettings>()
{
  return mmf_interfaces::msg::builder::Init_GetAppUpdateSettings_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS__BUILDER_HPP_
