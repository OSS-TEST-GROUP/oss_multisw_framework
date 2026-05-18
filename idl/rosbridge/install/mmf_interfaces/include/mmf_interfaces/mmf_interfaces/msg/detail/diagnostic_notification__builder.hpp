// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/DiagnosticNotification.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/diagnostic_notification__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_DiagnosticNotification_raw_data
{
public:
  explicit Init_DiagnosticNotification_raw_data(::mmf_interfaces::msg::DiagnosticNotification & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::DiagnosticNotification raw_data(::mmf_interfaces::msg::DiagnosticNotification::_raw_data_type arg)
  {
    msg_.raw_data = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

class Init_DiagnosticNotification_source_message_name
{
public:
  explicit Init_DiagnosticNotification_source_message_name(::mmf_interfaces::msg::DiagnosticNotification & msg)
  : msg_(msg)
  {}
  Init_DiagnosticNotification_raw_data source_message_name(::mmf_interfaces::msg::DiagnosticNotification::_source_message_name_type arg)
  {
    msg_.source_message_name = std::move(arg);
    return Init_DiagnosticNotification_raw_data(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

class Init_DiagnosticNotification_message
{
public:
  explicit Init_DiagnosticNotification_message(::mmf_interfaces::msg::DiagnosticNotification & msg)
  : msg_(msg)
  {}
  Init_DiagnosticNotification_source_message_name message(::mmf_interfaces::msg::DiagnosticNotification::_message_type arg)
  {
    msg_.message = std::move(arg);
    return Init_DiagnosticNotification_source_message_name(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

class Init_DiagnosticNotification_status
{
public:
  explicit Init_DiagnosticNotification_status(::mmf_interfaces::msg::DiagnosticNotification & msg)
  : msg_(msg)
  {}
  Init_DiagnosticNotification_message status(::mmf_interfaces::msg::DiagnosticNotification::_status_type arg)
  {
    msg_.status = std::move(arg);
    return Init_DiagnosticNotification_message(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

class Init_DiagnosticNotification_diag_name
{
public:
  explicit Init_DiagnosticNotification_diag_name(::mmf_interfaces::msg::DiagnosticNotification & msg)
  : msg_(msg)
  {}
  Init_DiagnosticNotification_status diag_name(::mmf_interfaces::msg::DiagnosticNotification::_diag_name_type arg)
  {
    msg_.diag_name = std::move(arg);
    return Init_DiagnosticNotification_status(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

class Init_DiagnosticNotification_diag_code
{
public:
  Init_DiagnosticNotification_diag_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DiagnosticNotification_diag_name diag_code(::mmf_interfaces::msg::DiagnosticNotification::_diag_code_type arg)
  {
    msg_.diag_code = std::move(arg);
    return Init_DiagnosticNotification_diag_name(msg_);
  }

private:
  ::mmf_interfaces::msg::DiagnosticNotification msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::DiagnosticNotification>()
{
  return mmf_interfaces::msg::builder::Init_DiagnosticNotification_diag_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__BUILDER_HPP_
