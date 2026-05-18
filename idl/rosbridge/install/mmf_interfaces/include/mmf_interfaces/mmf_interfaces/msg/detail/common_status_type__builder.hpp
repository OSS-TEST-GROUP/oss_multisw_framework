// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/CommonStatusType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/common_status_type__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_CommonStatusType_value
{
public:
  Init_CommonStatusType_value()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::mmf_interfaces::msg::CommonStatusType value(::mmf_interfaces::msg::CommonStatusType::_value_type arg)
  {
    msg_.value = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::CommonStatusType msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::CommonStatusType>()
{
  return mmf_interfaces::msg::builder::Init_CommonStatusType_value();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__COMMON_STATUS_TYPE__BUILDER_HPP_
