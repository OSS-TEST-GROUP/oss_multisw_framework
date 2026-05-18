// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/GetAppUpdateStateResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__GetAppUpdateStateResponse __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__GetAppUpdateStateResponse __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GetAppUpdateStateResponse_
{
  using Type = GetAppUpdateStateResponse_<ContainerAllocator>;

  explicit GetAppUpdateStateResponse_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->id = 0l;
      this->app_name = "";
      this->app_state_info = "";
      this->ret_code = 0l;
    }
  }

  explicit GetAppUpdateStateResponse_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : app_name(_alloc),
    app_state_info(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->id = 0l;
      this->app_name = "";
      this->app_state_info = "";
      this->ret_code = 0l;
    }
  }

  // field types and members
  using _id_type =
    int32_t;
  _id_type id;
  using _app_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _app_name_type app_name;
  using _app_state_info_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _app_state_info_type app_state_info;
  using _ret_code_type =
    int32_t;
  _ret_code_type ret_code;

  // setters for named parameter idiom
  Type & set__id(
    const int32_t & _arg)
  {
    this->id = _arg;
    return *this;
  }
  Type & set__app_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->app_name = _arg;
    return *this;
  }
  Type & set__app_state_info(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->app_state_info = _arg;
    return *this;
  }
  Type & set__ret_code(
    const int32_t & _arg)
  {
    this->ret_code = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__GetAppUpdateStateResponse
    std::shared_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__GetAppUpdateStateResponse
    std::shared_ptr<mmf_interfaces::msg::GetAppUpdateStateResponse_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetAppUpdateStateResponse_ & other) const
  {
    if (this->id != other.id) {
      return false;
    }
    if (this->app_name != other.app_name) {
      return false;
    }
    if (this->app_state_info != other.app_state_info) {
      return false;
    }
    if (this->ret_code != other.ret_code) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetAppUpdateStateResponse_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetAppUpdateStateResponse_

// alias to use template instance with default allocator
using GetAppUpdateStateResponse =
  mmf_interfaces::msg::GetAppUpdateStateResponse_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_APP_UPDATE_STATE_RESPONSE__STRUCT_HPP_
