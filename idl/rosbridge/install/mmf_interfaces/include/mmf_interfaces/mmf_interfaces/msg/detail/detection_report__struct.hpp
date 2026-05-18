// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/DetectionReport.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__DetectionReport __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__DetectionReport __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct DetectionReport_
{
  using Type = DetectionReport_<ContainerAllocator>;

  explicit DetectionReport_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_id = "";
      this->dds_topic = "";
      this->rule = "";
      this->timestamp_ms = 0ll;
    }
  }

  explicit DetectionReport_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : client_id(_alloc),
    dds_topic(_alloc),
    rule(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->client_id = "";
      this->dds_topic = "";
      this->rule = "";
      this->timestamp_ms = 0ll;
    }
  }

  // field types and members
  using _client_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _client_id_type client_id;
  using _dds_topic_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _dds_topic_type dds_topic;
  using _rule_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _rule_type rule;
  using _timestamp_ms_type =
    int64_t;
  _timestamp_ms_type timestamp_ms;

  // setters for named parameter idiom
  Type & set__client_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->client_id = _arg;
    return *this;
  }
  Type & set__dds_topic(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->dds_topic = _arg;
    return *this;
  }
  Type & set__rule(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->rule = _arg;
    return *this;
  }
  Type & set__timestamp_ms(
    const int64_t & _arg)
  {
    this->timestamp_ms = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::DetectionReport_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::DetectionReport_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DetectionReport_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::DetectionReport_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__DetectionReport
    std::shared_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__DetectionReport
    std::shared_ptr<mmf_interfaces::msg::DetectionReport_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const DetectionReport_ & other) const
  {
    if (this->client_id != other.client_id) {
      return false;
    }
    if (this->dds_topic != other.dds_topic) {
      return false;
    }
    if (this->rule != other.rule) {
      return false;
    }
    if (this->timestamp_ms != other.timestamp_ms) {
      return false;
    }
    return true;
  }
  bool operator!=(const DetectionReport_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct DetectionReport_

// alias to use template instance with default allocator
using DetectionReport =
  mmf_interfaces::msg::DetectionReport_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__DETECTION_REPORT__STRUCT_HPP_
