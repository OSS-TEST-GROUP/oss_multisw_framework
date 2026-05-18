// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/AdswServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/adsw_service_status__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_AdswServiceStatus_status_msg
{
public:
  explicit Init_AdswServiceStatus_status_msg(::mmf_interfaces::msg::AdswServiceStatus & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::AdswServiceStatus status_msg(::mmf_interfaces::msg::AdswServiceStatus::_status_msg_type arg)
  {
    msg_.status_msg = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::AdswServiceStatus msg_;
};

class Init_AdswServiceStatus_status_code
{
public:
  Init_AdswServiceStatus_status_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_AdswServiceStatus_status_msg status_code(::mmf_interfaces::msg::AdswServiceStatus::_status_code_type arg)
  {
    msg_.status_code = std::move(arg);
    return Init_AdswServiceStatus_status_msg(msg_);
  }

private:
  ::mmf_interfaces::msg::AdswServiceStatus msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::AdswServiceStatus>()
{
  return mmf_interfaces::msg::builder::Init_AdswServiceStatus_status_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__BUILDER_HPP_
