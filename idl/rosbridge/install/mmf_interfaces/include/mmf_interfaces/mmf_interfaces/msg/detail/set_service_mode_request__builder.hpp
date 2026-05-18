// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetServiceModeRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_service_mode_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetServiceModeRequest_service_mode
{
public:
  explicit Init_SetServiceModeRequest_service_mode(::mmf_interfaces::msg::SetServiceModeRequest & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetServiceModeRequest service_mode(::mmf_interfaces::msg::SetServiceModeRequest::_service_mode_type arg)
  {
    msg_.service_mode = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetServiceModeRequest msg_;
};

class Init_SetServiceModeRequest_is_group
{
public:
  explicit Init_SetServiceModeRequest_is_group(::mmf_interfaces::msg::SetServiceModeRequest & msg)
  : msg_(msg)
  {}
  Init_SetServiceModeRequest_service_mode is_group(::mmf_interfaces::msg::SetServiceModeRequest::_is_group_type arg)
  {
    msg_.is_group = std::move(arg);
    return Init_SetServiceModeRequest_service_mode(msg_);
  }

private:
  ::mmf_interfaces::msg::SetServiceModeRequest msg_;
};

class Init_SetServiceModeRequest_service_list
{
public:
  explicit Init_SetServiceModeRequest_service_list(::mmf_interfaces::msg::SetServiceModeRequest & msg)
  : msg_(msg)
  {}
  Init_SetServiceModeRequest_is_group service_list(::mmf_interfaces::msg::SetServiceModeRequest::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return Init_SetServiceModeRequest_is_group(msg_);
  }

private:
  ::mmf_interfaces::msg::SetServiceModeRequest msg_;
};

class Init_SetServiceModeRequest_os_type
{
public:
  Init_SetServiceModeRequest_os_type()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetServiceModeRequest_service_list os_type(::mmf_interfaces::msg::SetServiceModeRequest::_os_type_type arg)
  {
    msg_.os_type = std::move(arg);
    return Init_SetServiceModeRequest_service_list(msg_);
  }

private:
  ::mmf_interfaces::msg::SetServiceModeRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetServiceModeRequest>()
{
  return mmf_interfaces::msg::builder::Init_SetServiceModeRequest_os_type();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__BUILDER_HPP_
