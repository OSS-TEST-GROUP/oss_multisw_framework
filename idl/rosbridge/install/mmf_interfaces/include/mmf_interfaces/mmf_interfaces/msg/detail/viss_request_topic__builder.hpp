// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/VissRequestTopic.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/viss_request_topic__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_VissRequestTopic_payload
{
public:
  Init_VissRequestTopic_payload()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::mmf_interfaces::msg::VissRequestTopic payload(::mmf_interfaces::msg::VissRequestTopic::_payload_type arg)
  {
    msg_.payload = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::VissRequestTopic msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::VissRequestTopic>()
{
  return mmf_interfaces::msg::builder::Init_VissRequestTopic_payload();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__VISS_REQUEST_TOPIC__BUILDER_HPP_
