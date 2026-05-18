// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/AdswServiceStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__AdswServiceStatus __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__AdswServiceStatus __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct AdswServiceStatus_
{
  using Type = AdswServiceStatus_<ContainerAllocator>;

  explicit AdswServiceStatus_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status_code = 0l;
      this->status_msg = "";
    }
  }

  explicit AdswServiceStatus_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : status_msg(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status_code = 0l;
      this->status_msg = "";
    }
  }

  // field types and members
  using _status_code_type =
    int32_t;
  _status_code_type status_code;
  using _status_msg_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _status_msg_type status_msg;

  // setters for named parameter idiom
  Type & set__status_code(
    const int32_t & _arg)
  {
    this->status_code = _arg;
    return *this;
  }
  Type & set__status_msg(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->status_msg = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__AdswServiceStatus
    std::shared_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__AdswServiceStatus
    std::shared_ptr<mmf_interfaces::msg::AdswServiceStatus_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const AdswServiceStatus_ & other) const
  {
    if (this->status_code != other.status_code) {
      return false;
    }
    if (this->status_msg != other.status_msg) {
      return false;
    }
    return true;
  }
  bool operator!=(const AdswServiceStatus_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct AdswServiceStatus_

// alias to use template instance with default allocator
using AdswServiceStatus =
  mmf_interfaces::msg::AdswServiceStatus_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__ADSW_SERVICE_STATUS__STRUCT_HPP_
