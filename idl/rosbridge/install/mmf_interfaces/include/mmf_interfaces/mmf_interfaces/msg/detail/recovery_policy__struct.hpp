// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/RecoveryPolicy.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__RecoveryPolicy __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__RecoveryPolicy __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct RecoveryPolicy_
{
  using Type = RecoveryPolicy_<ContainerAllocator>;

  explicit RecoveryPolicy_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->value = 0l;
    }
  }

  explicit RecoveryPolicy_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->value = 0l;
    }
  }

  // field types and members
  using _value_type =
    int32_t;
  _value_type value;

  // setters for named parameter idiom
  Type & set__value(
    const int32_t & _arg)
  {
    this->value = _arg;
    return *this;
  }

  // constant declarations
  static constexpr int32_t R_DEFAULT =
    0;
  static constexpr int32_t ALWAYS =
    1;
  static constexpr int32_t ON_FAILURE =
    2;
  static constexpr int32_t ON_SUCCESS =
    3;
  static constexpr int32_t ON_NOT_RESPONDING =
    4;
  static constexpr int32_t MANUAL =
    5;

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__RecoveryPolicy
    std::shared_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__RecoveryPolicy
    std::shared_ptr<mmf_interfaces::msg::RecoveryPolicy_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const RecoveryPolicy_ & other) const
  {
    if (this->value != other.value) {
      return false;
    }
    return true;
  }
  bool operator!=(const RecoveryPolicy_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct RecoveryPolicy_

// alias to use template instance with default allocator
using RecoveryPolicy =
  mmf_interfaces::msg::RecoveryPolicy_<std::allocator<void>>;

// constant definitions
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::R_DEFAULT;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::ALWAYS;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::ON_FAILURE;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::ON_SUCCESS;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::ON_NOT_RESPONDING;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t RecoveryPolicy_<ContainerAllocator>::MANUAL;
#endif  // __cplusplus < 201703L

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__RECOVERY_POLICY__STRUCT_HPP_
