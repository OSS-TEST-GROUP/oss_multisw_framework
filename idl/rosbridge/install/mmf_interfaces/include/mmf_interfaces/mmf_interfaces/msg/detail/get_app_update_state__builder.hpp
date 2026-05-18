// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetAppUpdateState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_app_update_state__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetAppUpdateState_app_name
{
public:
  explicit Init_GetAppUpdateState_app_name(::mmf_interfaces::msg::GetAppUpdateState & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetAppUpdateState app_name(::mmf_interfaces::msg::GetAppUpdateState::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateState msg_;
};

class Init_GetAppUpdateState_id
{
public:
  Init_GetAppUpdateState_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetAppUpdateState_app_name id(::mmf_interfaces::msg::GetAppUpdateState::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetAppUpdateState_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateState msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetAppUpdateState>()
{
  return mmf_interfaces::msg::builder::Init_GetAppUpdateState_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE__BUILDER_HPP_
