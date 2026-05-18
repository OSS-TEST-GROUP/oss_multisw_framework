// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetAppUpdate.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_app_update__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetAppUpdate_app_name
{
public:
  explicit Init_SetAppUpdate_app_name(::mmf_interfaces::msg::SetAppUpdate & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetAppUpdate app_name(::mmf_interfaces::msg::SetAppUpdate::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdate msg_;
};

class Init_SetAppUpdate_id
{
public:
  Init_SetAppUpdate_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetAppUpdate_app_name id(::mmf_interfaces::msg::SetAppUpdate::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_SetAppUpdate_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdate msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetAppUpdate>()
{
  return mmf_interfaces::msg::builder::Init_SetAppUpdate_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE__BUILDER_HPP_
