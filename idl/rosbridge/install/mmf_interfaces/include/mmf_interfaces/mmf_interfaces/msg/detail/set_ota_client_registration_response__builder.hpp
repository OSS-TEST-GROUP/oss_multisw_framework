// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetOtaClientRegistrationResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_ota_client_registration_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetOtaClientRegistrationResponse_ret_code
{
public:
  explicit Init_SetOtaClientRegistrationResponse_ret_code(::mmf_interfaces::msg::SetOtaClientRegistrationResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::SetOtaClientRegistrationResponse ret_code(::mmf_interfaces::msg::SetOtaClientRegistrationResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetOtaClientRegistrationResponse msg_;
};

class Init_SetOtaClientRegistrationResponse_client_id
{
public:
  Init_SetOtaClientRegistrationResponse_client_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetOtaClientRegistrationResponse_ret_code client_id(::mmf_interfaces::msg::SetOtaClientRegistrationResponse::_client_id_type arg)
  {
    msg_.client_id = std::move(arg);
    return Init_SetOtaClientRegistrationResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::SetOtaClientRegistrationResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetOtaClientRegistrationResponse>()
{
  return mmf_interfaces::msg::builder::Init_SetOtaClientRegistrationResponse_client_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION_RESPONSE__BUILDER_HPP_
