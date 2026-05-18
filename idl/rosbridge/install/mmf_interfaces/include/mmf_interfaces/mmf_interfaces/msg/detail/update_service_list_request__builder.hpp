// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/UpdateServiceListRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/update_service_list_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_UpdateServiceListRequest_service_list
{
public:
  Init_UpdateServiceListRequest_service_list()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::mmf_interfaces::msg::UpdateServiceListRequest service_list(::mmf_interfaces::msg::UpdateServiceListRequest::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::UpdateServiceListRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::UpdateServiceListRequest>()
{
  return mmf_interfaces::msg::builder::Init_UpdateServiceListRequest_service_list();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__BUILDER_HPP_
