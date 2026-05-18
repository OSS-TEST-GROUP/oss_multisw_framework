// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/DiagnosticInfoRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/diagnostic_info_request__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_DiagnosticInfoRequest_diag_name
{
public:
  explicit Init_DiagnosticInfoRequest_diag_name(::mmf_interfaces::msg::DiagnosticInfoRequest & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::DiagnosticInfoRequest diag_name(::mmf_interfaces::msg::DiagnosticInfoRequest::_diag_name_type arg)
  {
    msg_.diag_name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticInfoRequest msg_;
};

class Init_DiagnosticInfoRequest_diag_code
{
public:
  Init_DiagnosticInfoRequest_diag_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DiagnosticInfoRequest_diag_name diag_code(::mmf_interfaces::msg::DiagnosticInfoRequest::_diag_code_type arg)
  {
    msg_.diag_code = std::move(arg);
    return Init_DiagnosticInfoRequest_diag_name(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticInfoRequest msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::DiagnosticInfoRequest>()
{
  return mmf_interfaces::msg::builder::Init_DiagnosticInfoRequest_diag_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_REQUEST__BUILDER_HPP_
