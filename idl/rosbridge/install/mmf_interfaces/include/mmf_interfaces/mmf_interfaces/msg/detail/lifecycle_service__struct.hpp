// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/LifecycleService.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__LifecycleService __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__LifecycleService __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct LifecycleService_
{
  using Type = LifecycleService_<ContainerAllocator>;

  explicit LifecycleService_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->name = "";
      this->service_type = 0l;
      this->recovery_policy = 0l;
      this->is_enabled = false;
      this->os_type = 0l;
    }
  }

  explicit LifecycleService_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : name(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->name = "";
      this->service_type = 0l;
      this->recovery_policy = 0l;
      this->is_enabled = false;
      this->os_type = 0l;
    }
  }

  // field types and members
  using _name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _name_type name;
  using _service_type_type =
    int32_t;
  _service_type_type service_type;
  using _recovery_policy_type =
    int32_t;
  _recovery_policy_type recovery_policy;
  using _is_enabled_type =
    bool;
  _is_enabled_type is_enabled;
  using _os_type_type =
    int32_t;
  _os_type_type os_type;

  // setters for named parameter idiom
  Type & set__name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->name = _arg;
    return *this;
  }
  Type & set__service_type(
    const int32_t & _arg)
  {
    this->service_type = _arg;
    return *this;
  }
  Type & set__recovery_policy(
    const int32_t & _arg)
  {
    this->recovery_policy = _arg;
    return *this;
  }
  Type & set__is_enabled(
    const bool & _arg)
  {
    this->is_enabled = _arg;
    return *this;
  }
  Type & set__os_type(
    const int32_t & _arg)
  {
    this->os_type = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::LifecycleService_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::LifecycleService_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__LifecycleService
    std::shared_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__LifecycleService
    std::shared_ptr<mmf_interfaces::msg::LifecycleService_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const LifecycleService_ & other) const
  {
    if (this->name != other.name) {
      return false;
    }
    if (this->service_type != other.service_type) {
      return false;
    }
    if (this->recovery_policy != other.recovery_policy) {
      return false;
    }
    if (this->is_enabled != other.is_enabled) {
      return false;
    }
    if (this->os_type != other.os_type) {
      return false;
    }
    return true;
  }
  bool operator!=(const LifecycleService_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct LifecycleService_

// alias to use template instance with default allocator
using LifecycleService =
  mmf_interfaces::msg::LifecycleService_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_SERVICE__STRUCT_HPP_
