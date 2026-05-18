// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/ServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/service_status__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_ServiceStatus_recovered_count
{
public:
  explicit Init_ServiceStatus_recovered_count(::mmf_interfaces::msg::ServiceStatus & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::ServiceStatus recovered_count(::mmf_interfaces::msg::ServiceStatus::_recovered_count_type arg)
  {
    msg_.recovered_count = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::ServiceStatus msg_;
};

class Init_ServiceStatus_is_enabled
{
public:
  explicit Init_ServiceStatus_is_enabled(::mmf_interfaces::msg::ServiceStatus & msg)
  : msg_(msg)
  {}
  Init_ServiceStatus_recovered_count is_enabled(::mmf_interfaces::msg::ServiceStatus::_is_enabled_type arg)
  {
    msg_.is_enabled = std::move(arg);
    return Init_ServiceStatus_recovered_count(msg_);
  }

private:
  ::mmf_interfaces::msg::ServiceStatus msg_;
};

class Init_ServiceStatus_service_state
{
public:
  explicit Init_ServiceStatus_service_state(::mmf_interfaces::msg::ServiceStatus & msg)
  : msg_(msg)
  {}
  Init_ServiceStatus_is_enabled service_state(::mmf_interfaces::msg::ServiceStatus::_service_state_type arg)
  {
    msg_.service_state = std::move(arg);
    return Init_ServiceStatus_is_enabled(msg_);
  }

private:
  ::mmf_interfaces::msg::ServiceStatus msg_;
};

class Init_ServiceStatus_service_type
{
public:
  explicit Init_ServiceStatus_service_type(::mmf_interfaces::msg::ServiceStatus & msg)
  : msg_(msg)
  {}
  Init_ServiceStatus_service_state service_type(::mmf_interfaces::msg::ServiceStatus::_service_type_type arg)
  {
    msg_.service_type = std::move(arg);
    return Init_ServiceStatus_service_state(msg_);
  }

private:
  ::mmf_interfaces::msg::ServiceStatus msg_;
};

class Init_ServiceStatus_name
{
public:
  Init_ServiceStatus_name()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_ServiceStatus_service_type name(::mmf_interfaces::msg::ServiceStatus::_name_type arg)
  {
    msg_.name = std::move(arg);
    return Init_ServiceStatus_service_type(msg_);
  }

private:
  ::mmf_interfaces::msg::ServiceStatus msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::ServiceStatus>()
{
  return mmf_interfaces::msg::builder::Init_ServiceStatus_name();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SERVICE_STATUS__BUILDER_HPP_
