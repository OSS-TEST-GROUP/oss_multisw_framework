// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/NotifyAppUpdateStatus.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__NotifyAppUpdateStatus __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__NotifyAppUpdateStatus __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct NotifyAppUpdateStatus_
{
  using Type = NotifyAppUpdateStatus_<ContainerAllocator>;

  explicit NotifyAppUpdateStatus_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_id = 0l;
      this->app_name = "";
      this->status = "";
    }
  }

  explicit NotifyAppUpdateStatus_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : app_name(_alloc),
    status(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_id = 0l;
      this->app_name = "";
      this->status = "";
    }
  }

  // field types and members
  using _client_id_type =
    int32_t;
  _client_id_type client_id;
  using _app_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _app_name_type app_name;
  using _status_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _status_type status;

  // setters for named parameter idiom
  Type & set__client_id(
    const int32_t & _arg)
  {
    this->client_id = _arg;
    return *this;
  }
  Type & set__app_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->app_name = _arg;
    return *this;
  }
  Type & set__status(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->status = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__NotifyAppUpdateStatus
    std::shared_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__NotifyAppUpdateStatus
    std::shared_ptr<mmf_interfaces::msg::NotifyAppUpdateStatus_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const NotifyAppUpdateStatus_ & other) const
  {
    if (this->client_id != other.client_id) {
      return false;
    }
    if (this->app_name != other.app_name) {
      return false;
    }
    if (this->status != other.status) {
      return false;
    }
    return true;
  }
  bool operator!=(const NotifyAppUpdateStatus_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct NotifyAppUpdateStatus_

// alias to use template instance with default allocator
using NotifyAppUpdateStatus =
  mmf_interfaces::msg::NotifyAppUpdateStatus_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_APP_UPDATE_STATUS__STRUCT_HPP_
