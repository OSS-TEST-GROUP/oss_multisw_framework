// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/SetServiceModeResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__SetServiceModeResponse __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__SetServiceModeResponse __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct SetServiceModeResponse_
{
  using Type = SetServiceModeResponse_<ContainerAllocator>;

  explicit SetServiceModeResponse_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->ret_code = 0l;
    }
  }

  explicit SetServiceModeResponse_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->ret_code = 0l;
    }
  }

  // field types and members
  using _ret_code_type =
    int32_t;
  _ret_code_type ret_code;

  // setters for named parameter idiom
  Type & set__ret_code(
    const int32_t & _arg)
  {
    this->ret_code = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__SetServiceModeResponse
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__SetServiceModeResponse
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeResponse_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SetServiceModeResponse_ & other) const
  {
    if (this->ret_code != other.ret_code) {
      return false;
    }
    return true;
  }
  bool operator!=(const SetServiceModeResponse_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SetServiceModeResponse_

// alias to use template instance with default allocator
using SetServiceModeResponse =
  mmf_interfaces::msg::SetServiceModeResponse_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_RESPONSE__STRUCT_HPP_
