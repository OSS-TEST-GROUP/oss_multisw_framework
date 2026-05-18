// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetOtaClientRegistration.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_ota_client_registration__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetOtaClientRegistration_app_list
{
public:
  explicit Init_SetOtaClientRegistration_app_list(::mmf_interfaces::msg::SetOtaClientRegistration & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetOtaClientRegistration app_list(::mmf_interfaces::msg::SetOtaClientRegistration::_app_list_type arg)
  {
    msg_.app_list = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetOtaClientRegistration msg_;
};

class Init_SetOtaClientRegistration_client_name
{
public:
  Init_SetOtaClientRegistration_client_name()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetOtaClientRegistration_app_list client_name(::mmf_interfaces::msg::SetOtaClientRegistration::_client_name_type arg)
  {
    msg_.client_name = std::move(arg);
    return Init_SetOtaClientRegistration_app_list(msg_);
  }

private:
  ::mmf_interfaces::msg::SetOtaClientRegistration msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetOtaClientRegistration>()
{
  return mmf_interfaces::msg::builder::Init_SetOtaClientRegistration_client_name();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__BUILDER_HPP_
