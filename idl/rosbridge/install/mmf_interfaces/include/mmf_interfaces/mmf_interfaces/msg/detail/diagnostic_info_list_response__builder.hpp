// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/DiagnosticInfoListResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/diagnostic_info_list_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_DiagnosticInfoListResponse_diag_names
{
public:
  explicit Init_DiagnosticInfoListResponse_diag_names(::mmf_interfaces::msg::DiagnosticInfoListResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::DiagnosticInfoListResponse diag_names(::mmf_interfaces::msg::DiagnosticInfoListResponse::_diag_names_type arg)
  {
    msg_.diag_names = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticInfoListResponse msg_;
};

class Init_DiagnosticInfoListResponse_diag_codes
{
public:
  explicit Init_DiagnosticInfoListResponse_diag_codes(::mmf_interfaces::msg::DiagnosticInfoListResponse & msg)
  : msg_(msg)
  {}
  Init_DiagnosticInfoListResponse_diag_names diag_codes(::mmf_interfaces::msg::DiagnosticInfoListResponse::_diag_codes_type arg)
  {
    msg_.diag_codes = std::move(arg);
    return Init_DiagnosticInfoListResponse_diag_names(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticInfoListResponse msg_;
};

class Init_DiagnosticInfoListResponse_ret_code
{
public:
  Init_DiagnosticInfoListResponse_ret_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DiagnosticInfoListResponse_diag_codes ret_code(::mmf_interfaces::msg::DiagnosticInfoListResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return Init_DiagnosticInfoListResponse_diag_codes(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticInfoListResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::DiagnosticInfoListResponse>()
{
  return mmf_interfaces::msg::builder::Init_DiagnosticInfoListResponse_ret_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_INFO_LIST_RESPONSE__BUILDER_HPP_
