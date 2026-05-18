// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetAppUpdateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_app_update_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetAppUpdateResponse_ret_code
{
public:
  explicit Init_SetAppUpdateResponse_ret_code(::mmf_interfaces::msg::SetAppUpdateResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetAppUpdateResponse ret_code(::mmf_interfaces::msg::SetAppUpdateResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateResponse msg_;
};

class Init_SetAppUpdateResponse_app_name
{
public:
  explicit Init_SetAppUpdateResponse_app_name(::mmf_interfaces::msg::SetAppUpdateResponse & msg)
  : msg_(msg)
  {}
  Init_SetAppUpdateResponse_ret_code app_name(::mmf_interfaces::msg::SetAppUpdateResponse::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return Init_SetAppUpdateResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateResponse msg_;
};

class Init_SetAppUpdateResponse_id
{
public:
  Init_SetAppUpdateResponse_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetAppUpdateResponse_app_name id(::mmf_interfaces::msg::SetAppUpdateResponse::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_SetAppUpdateResponse_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::SetAppUpdateResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetAppUpdateResponse>()
{
  return mmf_interfaces::msg::builder::Init_SetAppUpdateResponse_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_APP_UPDATE_RESPONSE__BUILDER_HPP_
