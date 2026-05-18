// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetAppUpdateStateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_app_update_state_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetAppUpdateStateResponse_ret_code
{
public:
  explicit Init_GetAppUpdateStateResponse_ret_code(::mmf_interfaces::msg::GetAppUpdateStateResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetAppUpdateStateResponse ret_code(::mmf_interfaces::msg::GetAppUpdateStateResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateStateResponse msg_;
};

class Init_GetAppUpdateStateResponse_app_state_info
{
public:
  explicit Init_GetAppUpdateStateResponse_app_state_info(::mmf_interfaces::msg::GetAppUpdateStateResponse & msg)
  : msg_(msg)
  {}
  Init_GetAppUpdateStateResponse_ret_code app_state_info(::mmf_interfaces::msg::GetAppUpdateStateResponse::_app_state_info_type arg)
  {
    msg_.app_state_info = std::move(arg);
    return Init_GetAppUpdateStateResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateStateResponse msg_;
};

class Init_GetAppUpdateStateResponse_app_name
{
public:
  explicit Init_GetAppUpdateStateResponse_app_name(::mmf_interfaces::msg::GetAppUpdateStateResponse & msg)
  : msg_(msg)
  {}
  Init_GetAppUpdateStateResponse_app_state_info app_name(::mmf_interfaces::msg::GetAppUpdateStateResponse::_app_name_type arg)
  {
    msg_.app_name = std::move(arg);
    return Init_GetAppUpdateStateResponse_app_state_info(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateStateResponse msg_;
};

class Init_GetAppUpdateStateResponse_id
{
public:
  Init_GetAppUpdateStateResponse_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetAppUpdateStateResponse_app_name id(::mmf_interfaces::msg::GetAppUpdateStateResponse::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetAppUpdateStateResponse_app_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetAppUpdateStateResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetAppUpdateStateResponse>()
{
  return mmf_interfaces::msg::builder::Init_GetAppUpdateStateResponse_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__BUILDER_HPP_
