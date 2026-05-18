// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/StopServiceRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_REQUEST__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/stop_service_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_StopServiceRequest_is_group
{
public:
  explicit Init_StopServiceRequest_is_group(::mmf_interfaces::msg::StopServiceRequest & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::StopServiceRequest is_group(::mmf_interfaces::msg::StopServiceRequest::_is_group_type arg)
  {
    msg_.is_group = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::StopServiceRequest msg_;
};

class Init_StopServiceRequest_service_list
{
public:
  explicit Init_StopServiceRequest_service_list(::mmf_interfaces::msg::StopServiceRequest & msg)
  : msg_(msg)
  {}
  Init_StopServiceRequest_is_group service_list(::mmf_interfaces::msg::StopServiceRequest::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return Init_StopServiceRequest_is_group(msg_);
  }

private:
  ::mmf_interfaces::msg::StopServiceRequest msg_;
};

class Init_StopServiceRequest_os_type
{
public:
  Init_StopServiceRequest_os_type()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_StopServiceRequest_service_list os_type(::mmf_interfaces::msg::StopServiceRequest::_os_type_type arg)
  {
    msg_.os_type = std::move(arg);
    return Init_StopServiceRequest_service_list(msg_);
  }

private:
  ::mmf_interfaces::msg::StopServiceRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::StopServiceRequest>()
{
  return mmf_interfaces::msg::builder::Init_StopServiceRequest_os_type();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_REQUEST__BUILDER_HPP_
