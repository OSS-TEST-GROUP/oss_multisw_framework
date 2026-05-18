// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/DetectionReport.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/detection_report__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_DetectionReport_timestamp_ms
{
public:
  explicit Init_DetectionReport_timestamp_ms(::mmf_interfaces::msg::DetectionReport & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::DetectionReport timestamp_ms(::mmf_interfaces::msg::DetectionReport::_timestamp_ms_type arg)
  {
    msg_.timestamp_ms = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::DetectionReport msg_;
};

class Init_DetectionReport_rule
{
public:
  explicit Init_DetectionReport_rule(::mmf_interfaces::msg::DetectionReport & msg)
  : msg_(msg)
  {}
  Init_DetectionReport_timestamp_ms rule(::mmf_interfaces::msg::DetectionReport::_rule_type arg)
  {
    msg_.rule = std::move(arg);
    return Init_DetectionReport_timestamp_ms(msg_);
  }

private:
  ::mmf_interfaces::msg::DetectionReport msg_;
};

class Init_DetectionReport_dds_topic
{
public:
  explicit Init_DetectionReport_dds_topic(::mmf_interfaces::msg::DetectionReport & msg)
  : msg_(msg)
  {}
  Init_DetectionReport_rule dds_topic(::mmf_interfaces::msg::DetectionReport::_dds_topic_type arg)
  {
    msg_.dds_topic = std::move(arg);
    return Init_DetectionReport_rule(msg_);
  }

private:
  ::mmf_interfaces::msg::DetectionReport msg_;
};

class Init_DetectionReport_client_id
{
public:
  Init_DetectionReport_client_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_DetectionReport_dds_topic client_id(::mmf_interfaces::msg::DetectionReport::_client_id_type arg)
  {
    msg_.client_id = std::move(arg);
    return Init_DetectionReport_dds_topic(msg_);
  }

private:
  ::mmf_interfaces::msg::DetectionReport msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::DetectionReport>()
{
  return mmf_interfaces::msg::builder::Init_DetectionReport_client_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__BUILDER_HPP_
