// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetVehicleInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_vehicle_info__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetVehicleInfo_name
{
public:
  explicit Init_GetVehicleInfo_name(::mmf_interfaces::msg::GetVehicleInfo & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetVehicleInfo name(::mmf_interfaces::msg::GetVehicleInfo::_name_type arg)
  {
    msg_.name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfo msg_;
};

class Init_GetVehicleInfo_id
{
public:
  Init_GetVehicleInfo_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetVehicleInfo_name id(::mmf_interfaces::msg::GetVehicleInfo::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetVehicleInfo_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfo msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetVehicleInfo>()
{
  return mmf_interfaces::msg::builder::Init_GetVehicleInfo_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO__BUILDER_HPP_
