// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/LifecycleDiagReport.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/lifecycle_diag_report__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_LifecycleDiagReport_service_list
{
public:
  explicit Init_LifecycleDiagReport_service_list(::mmf_interfaces::msg::LifecycleDiagReport & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::LifecycleDiagReport service_list(::mmf_interfaces::msg::LifecycleDiagReport::_service_list_type arg)
  {
    msg_.service_list = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleDiagReport msg_;
};

class Init_LifecycleDiagReport_total_services_num
{
public:
  explicit Init_LifecycleDiagReport_total_services_num(::mmf_interfaces::msg::LifecycleDiagReport & msg)
  : msg_(msg)
  {}
  Init_LifecycleDiagReport_service_list total_services_num(::mmf_interfaces::msg::LifecycleDiagReport::_total_services_num_type arg)
  {
    msg_.total_services_num = std::move(arg);
    return Init_LifecycleDiagReport_service_list(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleDiagReport msg_;
};

class Init_LifecycleDiagReport_running_service_num
{
public:
  explicit Init_LifecycleDiagReport_running_service_num(::mmf_interfaces::msg::LifecycleDiagReport & msg)
  : msg_(msg)
  {}
  Init_LifecycleDiagReport_total_services_num running_service_num(::mmf_interfaces::msg::LifecycleDiagReport::_running_service_num_type arg)
  {
    msg_.running_service_num = std::move(arg);
    return Init_LifecycleDiagReport_total_services_num(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleDiagReport msg_;
};

class Init_LifecycleDiagReport_is_started
{
public:
  Init_LifecycleDiagReport_is_started()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_LifecycleDiagReport_running_service_num is_started(::mmf_interfaces::msg::LifecycleDiagReport::_is_started_type arg)
  {
    msg_.is_started = std::move(arg);
    return Init_LifecycleDiagReport_running_service_num(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleDiagReport msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::LifecycleDiagReport>()
{
  return mmf_interfaces::msg::builder::Init_LifecycleDiagReport_is_started();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__BUILDER_HPP_
