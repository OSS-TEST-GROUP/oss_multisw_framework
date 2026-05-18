// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/LifecycleReturnCode.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/lifecycle_return_code__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_LifecycleReturnCode_value
{
public:
  Init_LifecycleReturnCode_value()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::mmf_interfaces::msg::LifecycleReturnCode value(::mmf_interfaces::msg::LifecycleReturnCode::_value_type arg)
  {
    msg_.value = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleReturnCode msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::LifecycleReturnCode>()
{
  return mmf_interfaces::msg::builder::Init_LifecycleReturnCode_value();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_RETURN_CODE__BUILDER_HPP_
