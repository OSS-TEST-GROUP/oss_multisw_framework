// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/SetOtaClientRegistration.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__SetOtaClientRegistration __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__SetOtaClientRegistration __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct SetOtaClientRegistration_
{
  using Type = SetOtaClientRegistration_<ContainerAllocator>;

  explicit SetOtaClientRegistration_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_name = "";
    }
  }

  explicit SetOtaClientRegistration_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : client_name(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_name = "";
    }
  }

  // field types and members
  using _client_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _client_name_type client_name;
  using _app_list_type =
    std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>>;
  _app_list_type app_list;

  // setters for named parameter idiom
  Type & set__client_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->client_name = _arg;
    return *this;
  }
  Type & set__app_list(
    const std::vector<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>>> & _arg)
  {
    this->app_list = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__SetOtaClientRegistration
    std::shared_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__SetOtaClientRegistration
    std::shared_ptr<mmf_interfaces::msg::SetOtaClientRegistration_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SetOtaClientRegistration_ & other) const
  {
    if (this->client_name != other.client_name) {
      return false;
    }
    if (this->app_list != other.app_list) {
      return false;
    }
    return true;
  }
  bool operator!=(const SetOtaClientRegistration_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SetOtaClientRegistration_

// alias to use template instance with default allocator
using SetOtaClientRegistration =
  mmf_interfaces::msg::SetOtaClientRegistration_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__SET_OTA_CLIENT_REGISTRATION__STRUCT_HPP_
