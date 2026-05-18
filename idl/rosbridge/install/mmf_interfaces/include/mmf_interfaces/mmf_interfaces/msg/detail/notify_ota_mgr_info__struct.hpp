// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/NotifyOtaMgrInfo.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__NotifyOtaMgrInfo __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__NotifyOtaMgrInfo __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct NotifyOtaMgrInfo_
{
  using Type = NotifyOtaMgrInfo_<ContainerAllocator>;

  explicit NotifyOtaMgrInfo_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->err_code = 0l;
      this->err_msg = "";
    }
  }

  explicit NotifyOtaMgrInfo_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : err_msg(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->err_code = 0l;
      this->err_msg = "";
    }
  }

  // field types and members
  using _err_code_type =
    int32_t;
  _err_code_type err_code;
  using _err_msg_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _err_msg_type err_msg;

  // setters for named parameter idiom
  Type & set__err_code(
    const int32_t & _arg)
  {
    this->err_code = _arg;
    return *this;
  }
  Type & set__err_msg(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->err_msg = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__NotifyOtaMgrInfo
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__NotifyOtaMgrInfo
    std::shared_ptr<mmf_interfaces::msg::NotifyOtaMgrInfo_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const NotifyOtaMgrInfo_ & other) const
  {
    if (this->err_code != other.err_code) {
      return false;
    }
    if (this->err_msg != other.err_msg) {
      return false;
    }
    return true;
  }
  bool operator!=(const NotifyOtaMgrInfo_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct NotifyOtaMgrInfo_

// alias to use template instance with default allocator
using NotifyOtaMgrInfo =
  mmf_interfaces::msg::NotifyOtaMgrInfo_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_OTA_MGR_INFO__STRUCT_HPP_
