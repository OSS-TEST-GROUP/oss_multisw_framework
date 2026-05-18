// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/DiagnosticStatusType.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__DiagnosticStatusType __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__DiagnosticStatusType __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct DiagnosticStatusType_
{
  using Type = DiagnosticStatusType_<ContainerAllocator>;

  explicit DiagnosticStatusType_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->value = 0l;
    }
  }

  explicit DiagnosticStatusType_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
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
  static constexpr int32_t UNKNOWN =
    0;
  static constexpr int32_t PASS =
    1;
  static constexpr int32_t FATAL =
    2;
  static constexpr int32_t WARNING =
    3;

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__DiagnosticStatusType
    std::shared_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__DiagnosticStatusType
    std::shared_ptr<mmf_interfaces::msg::DiagnosticStatusType_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DiagnosticStatusType_ & other) const
  {
    if (this->value != other.value) {
      return false;
    }
    return true;
  }
  bool operator!=(const DiagnosticStatusType_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DiagnosticStatusType_

// alias to use template instance with default allocator
using DiagnosticStatusType =
  mmf_interfaces::msg::DiagnosticStatusType_<std::allocator<void>>;

// constant definitions
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t DiagnosticStatusType_<ContainerAllocator>::UNKNOWN;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t DiagnosticStatusType_<ContainerAllocator>::PASS;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t DiagnosticStatusType_<ContainerAllocator>::FATAL;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t DiagnosticStatusType_<ContainerAllocator>::WARNING;
#endif  // __cplusplus < 201703L

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_STATUS_TYPE__STRUCT_HPP_
