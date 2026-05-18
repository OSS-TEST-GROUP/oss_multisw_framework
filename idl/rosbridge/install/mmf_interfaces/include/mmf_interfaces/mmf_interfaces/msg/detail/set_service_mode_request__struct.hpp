// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/SetServiceModeRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__SetServiceModeRequest __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__SetServiceModeRequest __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct SetServiceModeRequest_
{
  using Type = SetServiceModeRequest_<ContainerAllocator>;

  explicit SetServiceModeRequest_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->os_type = 0l;
      this->is_group = false;
      this->service_mode = 0l;
    }
  }

  explicit SetServiceModeRequest_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->os_type = 0l;
      this->is_group = false;
      this->service_mode = 0l;
    }
  }

  // field types and members
  using _os_type_type =
    int32_t;
  _os_type_type os_type;
  using _service_list_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _service_list_type service_list;
  using _is_group_type =
    bool;
  _is_group_type is_group;
  using _service_mode_type =
    int32_t;
  _service_mode_type service_mode;

  // setters for named parameter idiom
  Type & set__os_type(
    const int32_t & _arg)
  {
    this->os_type = _arg;
    return *this;
  }
  Type & set__service_list(
    const std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>> & _arg)
  {
    this->service_list = _arg;
    return *this;
  }
  Type & set__is_group(
    const bool & _arg)
  {
    this->is_group = _arg;
    return *this;
  }
  Type & set__service_mode(
    const int32_t & _arg)
  {
    this->service_mode = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__SetServiceModeRequest
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__SetServiceModeRequest
    std::shared_ptr<mmf_interfaces::msg::SetServiceModeRequest_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SetServiceModeRequest_ & other) const
  {
    if (this->os_type != other.os_type) {
      return false;
    }
    if (this->service_list != other.service_list) {
      return false;
    }
    if (this->is_group != other.is_group) {
      return false;
    }
    if (this->service_mode != other.service_mode) {
      return false;
    }
    return true;
  }
  bool operator!=(const SetServiceModeRequest_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SetServiceModeRequest_

// alias to use template instance with default allocator
using SetServiceModeRequest =
  mmf_interfaces::msg::SetServiceModeRequest_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_SERVICE_MODE_REQUEST__STRUCT_HPP_
