// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/GetServiceStateRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__GetServiceStateRequest __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__GetServiceStateRequest __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GetServiceStateRequest_
{
  using Type = GetServiceStateRequest_<ContainerAllocator>;

  explicit GetServiceStateRequest_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->os_type = 0l;
      this->is_all_service = false;
    }
  }

  explicit GetServiceStateRequest_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->os_type = 0l;
      this->is_all_service = false;
    }
  }

  // field types and members
  using _os_type_type =
    int32_t;
  _os_type_type os_type;
  using _service_list_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _service_list_type service_list;
  using _is_all_service_type =
    bool;
  _is_all_service_type is_all_service;

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
  Type & set__is_all_service(
    const bool & _arg)
  {
    this->is_all_service = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__GetServiceStateRequest
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__GetServiceStateRequest
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateRequest_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetServiceStateRequest_ & other) const
  {
    if (this->os_type != other.os_type) {
      return false;
    }
    if (this->service_list != other.service_list) {
      return false;
    }
    if (this->is_all_service != other.is_all_service) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetServiceStateRequest_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetServiceStateRequest_

// alias to use template instance with default allocator
using GetServiceStateRequest =
  mmf_interfaces::msg::GetServiceStateRequest_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_REQUEST__STRUCT_HPP_
