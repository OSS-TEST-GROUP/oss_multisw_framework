// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/DiagnosticNotification.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__DiagnosticNotification __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__DiagnosticNotification __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct DiagnosticNotification_
{
  using Type = DiagnosticNotification_<ContainerAllocator>;

  explicit DiagnosticNotification_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->diag_code = 0l;
      this->diag_name = "";
      this->status = 0l;
      this->message = "";
      this->source_message_name = "";
      this->raw_data = "";
    }
  }

  explicit DiagnosticNotification_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : diag_name(_alloc),
    message(_alloc),
    source_message_name(_alloc),
    raw_data(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->diag_code = 0l;
      this->diag_name = "";
      this->status = 0l;
      this->message = "";
      this->source_message_name = "";
      this->raw_data = "";
    }
  }

  // field types and members
  using _diag_code_type =
    int32_t;
  _diag_code_type diag_code;
  using _diag_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _diag_name_type diag_name;
  using _status_type =
    int32_t;
  _status_type status;
  using _message_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _message_type message;
  using _source_message_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _source_message_name_type source_message_name;
  using _raw_data_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _raw_data_type raw_data;

  // setters for named parameter idiom
  Type & set__diag_code(
    const int32_t & _arg)
  {
    this->diag_code = _arg;
    return *this;
  }
  Type & set__diag_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->diag_name = _arg;
    return *this;
  }
  Type & set__status(
    const int32_t & _arg)
  {
    this->status = _arg;
    return *this;
  }
  Type & set__message(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->message = _arg;
    return *this;
  }
  Type & set__source_message_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->source_message_name = _arg;
    return *this;
  }
  Type & set__raw_data(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->raw_data = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__DiagnosticNotification
    std::shared_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__DiagnosticNotification
    std::shared_ptr<mmf_interfaces::msg::DiagnosticNotification_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DiagnosticNotification_ & other) const
  {
    if (this->diag_code != other.diag_code) {
      return false;
    }
    if (this->diag_name != other.diag_name) {
      return false;
    }
    if (this->status != other.status) {
      return false;
    }
    if (this->message != other.message) {
      return false;
    }
    if (this->source_message_name != other.source_message_name) {
      return false;
    }
    if (this->raw_data != other.raw_data) {
      return false;
    }
    return true;
  }
  bool operator!=(const DiagnosticNotification_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DiagnosticNotification_

// alias to use template instance with default allocator
using DiagnosticNotification =
  mmf_interfaces::msg::DiagnosticNotification_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DIAGNOSTIC_NOTIFICATION__STRUCT_HPP_
