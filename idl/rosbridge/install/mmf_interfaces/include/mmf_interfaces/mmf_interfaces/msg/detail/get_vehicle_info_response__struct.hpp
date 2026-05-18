// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/GetVehicleInfoResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__GetVehicleInfoResponse __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__GetVehicleInfoResponse __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GetVehicleInfoResponse_
{
  using Type = GetVehicleInfoResponse_<ContainerAllocator>;

  explicit GetVehicleInfoResponse_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->id = 0l;
      this->name = "";
      this->value = "";
      this->ret_code = 0l;
    }
  }

  explicit GetVehicleInfoResponse_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : name(_alloc),
    value(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->id = 0l;
      this->name = "";
      this->value = "";
      this->ret_code = 0l;
    }
  }

  // field types and members
  using _id_type =
    int32_t;
  _id_type id;
  using _name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _name_type name;
  using _value_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _value_type value;
  using _ret_code_type =
    int32_t;
  _ret_code_type ret_code;

  // setters for named parameter idiom
  Type & set__id(
    const int32_t & _arg)
  {
    this->id = _arg;
    return *this;
  }
  Type & set__name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->name = _arg;
    return *this;
  }
  Type & set__value(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->value = _arg;
    return *this;
  }
  Type & set__ret_code(
    const int32_t & _arg)
  {
    this->ret_code = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__GetVehicleInfoResponse
    std::shared_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__GetVehicleInfoResponse
    std::shared_ptr<mmf_interfaces::msg::GetVehicleInfoResponse_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetVehicleInfoResponse_ & other) const
  {
    if (this->id != other.id) {
      return false;
    }
    if (this->name != other.name) {
      return false;
    }
    if (this->value != other.value) {
      return false;
    }
    if (this->ret_code != other.ret_code) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetVehicleInfoResponse_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetVehicleInfoResponse_

// alias to use template instance with default allocator
using GetVehicleInfoResponse =
  mmf_interfaces::msg::GetVehicleInfoResponse_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__STRUCT_HPP_
