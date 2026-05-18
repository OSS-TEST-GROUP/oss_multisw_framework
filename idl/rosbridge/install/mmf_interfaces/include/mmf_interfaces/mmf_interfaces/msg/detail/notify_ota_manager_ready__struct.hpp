// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/NotifyOtaManagerReady.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MANAGER_READY__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MANAGER_READY__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__NotifyOtaManagerReady __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__NotifyOtaManagerReady __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct NotifyOtaManagerReady_
{
  using Type = NotifyOtaManagerReady_<ContainerAllocator>;

  explicit NotifyOtaManagerReady_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->structure_needs_at_least_one_member = 0;
    }
  }

  explicit NotifyOtaManagerReady_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->structure_needs_at_least_one_member = 0;
    }
  }

  // field types and members
  using _structure_needs_at_least_one_member_type =
    uint8_t;
  _structure_needs_at_least_one_member_type structure_needs_at_least_one_member;


  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__NotifyOtaManagerReady
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__NotifyOtaManagerReady
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaManagerReady_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const NotifyOtaManagerReady_ & other) const
  {
    if (this->structure_needs_at_least_one_member != other.structure_needs_at_least_one_member) {
      return false;
    }
    return true;
  }
  bool operator!=(const NotifyOtaManagerReady_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct NotifyOtaManagerReady_

// alias to use template instance with default allocator
using NotifyOtaManagerReady =
  mmf_interfaces::msg::NotifyOtaManagerReady_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MANAGER_READY__STRUCT_HPP_
