// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetAppUpdateSettingsResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_app_update_settings_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetAppUpdateSettingsResponse_ret_code
{
public:
  explicit Init_SetAppUpdateSettingsResponse_ret_code(::mmf_interfaces::msg::SetAppUpdateSettingsResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetAppUpdateSettingsResponse ret_code(::mmf_interfaces::msg::SetAppUpdateSettingsResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateSettingsResponse msg_;
};

class Init_SetAppUpdateSettingsResponse_id
{
public:
  Init_SetAppUpdateSettingsResponse_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetAppUpdateSettingsResponse_ret_code id(::mmf_interfaces::msg::SetAppUpdateSettingsResponse::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_SetAppUpdateSettingsResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateSettingsResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetAppUpdateSettingsResponse>()
{
  return mmf_interfaces::msg::builder::Init_SetAppUpdateSettingsResponse_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_SETTINGS_RESPONSE__BUILDER_HPP_
