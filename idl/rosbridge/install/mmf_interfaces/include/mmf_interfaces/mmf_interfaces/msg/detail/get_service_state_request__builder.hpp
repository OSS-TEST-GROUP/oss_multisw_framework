// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetServiceStateRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_service_state_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetServiceStateRequest_is_all_service
{
public:
  explicit Init_GetServiceStateRequest_is_all_service(::mmf_interfaces::msg::GetServiceStateRequest & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetServiceStateRequest is_all_service(::mmf_interfaces::msg::GetServiceStateRequest::_is_all_service_type arg)
  {
    msg_.is_all_service = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetServiceStateRequest msg_;
};

class Init_GetServiceStateRequest_service_list
{
public:
  explicit Init_GetServiceStateRequest_service_list(::mmf_interfaces::msg::GetServiceStateRequest & msg)
  : msg_(msg)
  {}
  Init_GetServiceStateRequest_is_all_service service_list(::mmf_interfaces::msg::GetServiceStateRequest::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return Init_GetServiceStateRequest_is_all_service(msg_);
  }

private:
  ::mmf_interfaces::msg::GetServiceStateRequest msg_;
};

class Init_GetServiceStateRequest_os_type
{
public:
  Init_GetServiceStateRequest_os_type()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetServiceStateRequest_service_list os_type(::mmf_interfaces::msg::GetServiceStateRequest::_os_type_type arg)
  {
    msg_.os_type = std::move(arg);
    return Init_GetServiceStateRequest_service_list(msg_);
  }

private:
  ::mmf_interfaces::msg::GetServiceStateRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetServiceStateRequest>()
{
  return mmf_interfaces::msg::builder::Init_GetServiceStateRequest_os_type();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__BUILDER_HPP_
