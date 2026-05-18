// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/GetServiceStateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__GetServiceStateResponse __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__GetServiceStateResponse __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GetServiceStateResponse_
{
  using Type = GetServiceStateResponse_<ContainerAllocator>;

  explicit GetServiceStateResponse_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_init;
  }

  explicit GetServiceStateResponse_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_init;
    (void)_alloc;
  }

  // field types and members
  using _service_list_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _service_list_type service_list;
  using _service_state_list_type =
    std::vector<int32_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<int32_t>>;
  _service_state_list_type service_state_list;

  // setters for named parameter idiom
  Type & set__service_list(
    const std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>> & _arg)
  {
    this->service_list = _arg;
    return *this;
  }
  Type & set__service_state_list(
    const std::vector<int32_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<int32_t>> & _arg)
  {
    this->service_state_list = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__GetServiceStateResponse
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__GetServiceStateResponse
    std::shared_ptr<mmf_interfaces::msg::GetServiceStateResponse_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetServiceStateResponse_ & other) const
  {
    if (this->service_list != other.service_list) {
      return false;
    }
    if (this->service_state_list != other.service_state_list) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetServiceStateResponse_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetServiceStateResponse_

// alias to use template instance with default allocator
using GetServiceStateResponse =
  mmf_interfaces::msg::GetServiceStateResponse_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_SERVICE_STATE_RESPONSE__STRUCT_HPP_
