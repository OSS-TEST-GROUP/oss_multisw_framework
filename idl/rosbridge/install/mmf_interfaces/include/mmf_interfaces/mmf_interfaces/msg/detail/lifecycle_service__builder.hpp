// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/LifecycleService.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/lifecycle_service__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_LifecycleService_os_type
{
public:
  explicit Init_LifecycleService_os_type(::mmf_interfaces::msg::LifecycleService & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::LifecycleService os_type(::mmf_interfaces::msg::LifecycleService::_os_type_type arg)
  {
    msg_.os_type = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleService msg_;
};

class Init_LifecycleService_is_enabled
{
public:
  explicit Init_LifecycleService_is_enabled(::mmf_interfaces::msg::LifecycleService & msg)
  : msg_(msg)
  {}
  Init_LifecycleService_os_type is_enabled(::mmf_interfaces::msg::LifecycleService::_is_enabled_type arg)
  {
    msg_.is_enabled = std::move(arg);
    return Init_LifecycleService_os_type(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleService msg_;
};

class Init_LifecycleService_recovery_policy
{
public:
  explicit Init_LifecycleService_recovery_policy(::mmf_interfaces::msg::LifecycleService & msg)
  : msg_(msg)
  {}
  Init_LifecycleService_is_enabled recovery_policy(::mmf_interfaces::msg::LifecycleService::_recovery_policy_type arg)
  {
    msg_.recovery_policy = std::move(arg);
    return Init_LifecycleService_is_enabled(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleService msg_;
};

class Init_LifecycleService_service_type
{
public:
  explicit Init_LifecycleService_service_type(::mmf_interfaces::msg::LifecycleService & msg)
  : msg_(msg)
  {}
  Init_LifecycleService_recovery_policy service_type(::mmf_interfaces::msg::LifecycleService::_service_type_type arg)
  {
    msg_.service_type = std::move(arg);
    return Init_LifecycleService_recovery_policy(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleService msg_;
};

class Init_LifecycleService_name
{
public:
  Init_LifecycleService_name()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_LifecycleService_service_type name(::mmf_interfaces::msg::LifecycleService::_name_type arg)
  {
    msg_.name = std::move(arg);
    return Init_LifecycleService_service_type(msg_);
  }

private:
  ::mmf_interfaces::msg::LifecycleService msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::LifecycleService>()
{
  return mmf_interfaces::msg::builder::Init_LifecycleService_name();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__BUILDER_HPP_
