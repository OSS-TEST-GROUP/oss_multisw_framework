// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetServiceStateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_service_state_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetServiceStateResponse_service_state_list
{
public:
  explicit Init_GetServiceStateResponse_service_state_list(::mmf_interfaces::msg::GetServiceStateResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetServiceStateResponse service_state_list(::mmf_interfaces::msg::GetServiceStateResponse::_service_state_list_type arg)
  {
    msg_.service_state_list = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetServiceStateResponse msg_;
};

class Init_GetServiceStateResponse_service_list
{
public:
  Init_GetServiceStateResponse_service_list()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetServiceStateResponse_service_state_list service_list(::mmf_interfaces::msg::GetServiceStateResponse::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return Init_GetServiceStateResponse_service_state_list(msg_);
  }

private:
  ::mmf_interfaces::msg::GetServiceStateResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetServiceStateResponse>()
{
  return mmf_interfaces::msg::builder::Init_GetServiceStateResponse_service_list();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__BUILDER_HPP_
