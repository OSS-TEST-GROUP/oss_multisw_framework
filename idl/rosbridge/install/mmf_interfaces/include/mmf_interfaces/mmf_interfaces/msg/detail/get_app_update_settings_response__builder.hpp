// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetAppUpdateSettingsResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_app_update_settings_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetAppUpdateSettingsResponse_ret_code
{
public:
  explicit Init_GetAppUpdateSettingsResponse_ret_code(::mmf_interfaces::msg::GetAppUpdateSettingsResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetAppUpdateSettingsResponse ret_code(::mmf_interfaces::msg::GetAppUpdateSettingsResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettingsResponse msg_;
};

class Init_GetAppUpdateSettingsResponse_app_settings
{
public:
  explicit Init_GetAppUpdateSettingsResponse_app_settings(::mmf_interfaces::msg::GetAppUpdateSettingsResponse & msg)
  : msg_(msg)
  {}
  Init_GetAppUpdateSettingsResponse_ret_code app_settings(::mmf_interfaces::msg::GetAppUpdateSettingsResponse::_app_settings_type arg)
  {
    msg_.app_settings = std::move(arg);
    return Init_GetAppUpdateSettingsResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettingsResponse msg_;
};

class Init_GetAppUpdateSettingsResponse_app_name
{
public:
  explicit Init_GetAppUpdateSettingsResponse_app_name(::mmf_interfaces::msg::GetAppUpdateSettingsResponse & msg)
  : msg_(msg)
  {}
  Init_GetAppUpdateSettingsResponse_app_settings app_name(::mmf_interfaces::msg::GetAppUpdateSettingsResponse::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return Init_GetAppUpdateSettingsResponse_app_settings(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettingsResponse msg_;
};

class Init_GetAppUpdateSettingsResponse_id
{
public:
  Init_GetAppUpdateSettingsResponse_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetAppUpdateSettingsResponse_app_name id(::mmf_interfaces::msg::GetAppUpdateSettingsResponse::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetAppUpdateSettingsResponse_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateSettingsResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetAppUpdateSettingsResponse>()
{
  return mmf_interfaces::msg::builder::Init_GetAppUpdateSettingsResponse_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_
