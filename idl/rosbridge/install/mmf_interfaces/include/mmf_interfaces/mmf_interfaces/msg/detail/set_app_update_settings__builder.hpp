// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetAppUpdateSettings.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_app_update_settings__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetAppUpdateSettings_app_settings
{
public:
  explicit Init_SetAppUpdateSettings_app_settings(::mmf_interfaces::msg::SetAppUpdateSettings & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetAppUpdateSettings app_settings(::mmf_interfaces::msg::SetAppUpdateSettings::_app_settings_type arg)
  {
    msg_.app_settings = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateSettings msg_;
};

class Init_SetAppUpdateSettings_app_name
{
public:
  explicit Init_SetAppUpdateSettings_app_name(::mmf_interfaces::msg::SetAppUpdateSettings & msg)
  : msg_(msg)
  {}
  Init_SetAppUpdateSettings_app_settings app_name(::mmf_interfaces::msg::SetAppUpdateSettings::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return Init_SetAppUpdateSettings_app_settings(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateSettings msg_;
};

class Init_SetAppUpdateSettings_id
{
public:
  Init_SetAppUpdateSettings_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetAppUpdateSettings_app_name id(::mmf_interfaces::msg::SetAppUpdateSettings::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_SetAppUpdateSettings_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateSettings msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetAppUpdateSettings>()
{
  return mmf_interfaces::msg::builder::Init_SetAppUpdateSettings_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS__BUILDER_HPP_
