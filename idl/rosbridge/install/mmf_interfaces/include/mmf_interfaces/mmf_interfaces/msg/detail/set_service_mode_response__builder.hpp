// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/SetServiceModeResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/set_service_mode_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_SetServiceModeResponse_ret_code
{
public:
  Init_SetServiceModeResponse_ret_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::mmf_interfaces::msg::SetServiceModeResponse ret_code(::mmf_interfaces::msg::SetServiceModeResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::SetServiceModeResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::SetServiceModeResponse>()
{
  return mmf_interfaces::msg::builder::Init_SetServiceModeResponse_ret_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__BUILDER_HPP_
