// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mmf_interfaces:msg/GetVehicleInfoResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__BUILDER_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mmf_interfaces/msg/detail/get_vehicle_info_response__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mmf_interfaces
{

namespace msg
{

namespace builder
{

class Init_GetVehicleInfoResponse_ret_code
{
public:
  explicit Init_GetVehicleInfoResponse_ret_code(::mmf_interfaces::msg::GetVehicleInfoResponse & msg)
  : msg_(msg)
  {}
  ::mmf_interfaces::msg::GetVehicleInfoResponse ret_code(::mmf_interfaces::msg::GetVehicleInfoResponse::_ret_code_type arg)
  {
    msg_.ret_code = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfoResponse msg_;
};

class Init_GetVehicleInfoResponse_value
{
public:
  explicit Init_GetVehicleInfoResponse_value(::mmf_interfaces::msg::GetVehicleInfoResponse & msg)
  : msg_(msg)
  {}
  Init_GetVehicleInfoResponse_ret_code value(::mmf_interfaces::msg::GetVehicleInfoResponse::_value_type arg)
  {
    msg_.value = std::move(arg);
    return Init_GetVehicleInfoResponse_ret_code(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfoResponse msg_;
};

class Init_GetVehicleInfoResponse_name
{
public:
  explicit Init_GetVehicleInfoResponse_name(::mmf_interfaces::msg::GetVehicleInfoResponse & msg)
  : msg_(msg)
  {}
  Init_GetVehicleInfoResponse_value name(::mmf_interfaces::msg::GetVehicleInfoResponse::_name_type arg)
  {
    msg_.name = std::move(arg);
    return Init_GetVehicleInfoResponse_value(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfoResponse msg_;
};

class Init_GetVehicleInfoResponse_id
{
public:
  Init_GetVehicleInfoResponse_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetVehicleInfoResponse_name id(::mmf_interfaces::msg::GetVehicleInfoResponse::_id_type arg)
  {
    msg_.id = std::move(arg);
    return Init_GetVehicleInfoResponse_name(msg_);
  }

private:
  ::mmf_interfaces::msg::GetVehicleInfoResponse msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mmf_interfaces::msg::GetVehicleInfoResponse>()
{
  return mmf_interfaces::msg::builder::Init_GetVehicleInfoResponse_id();
}

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__BUILDER_HPP_
