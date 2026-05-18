// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/NotifyOtaMgrInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/notify_ota_mgr_info__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_NotifyOtaMgrInfo_err_msg
{
public:
  explicit Init_NotifyOtaMgrInfo_err_msg(::mmf_interfaces::msg::NotifyOtaMgrInfo & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::NotifyOtaMgrInfo err_msg(::mmf_interfaces::msg::NotifyOtaMgrInfo::_err_msg_type arg)
  {
    msg_.err_msg = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyOtaMgrInfo msg_;
};

class Init_NotifyOtaMgrInfo_err_code
{
public:
  Init_NotifyOtaMgrInfo_err_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_NotifyOtaMgrInfo_err_msg err_code(::mmf_interfaces::msg::NotifyOtaMgrInfo::_err_code_type arg)
  {
    msg_.err_code = std::move(arg);
    return Init_NotifyOtaMgrInfo_err_msg(msg_);
  }

private:
  ::mmf_interfaces::msg::NotifyOtaMgrInfo msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::NotifyOtaMgrInfo>()
{
  return mmf_interfaces::msg::builder::Init_NotifyOtaMgrInfo_err_code();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__BUILDER_HPP_
