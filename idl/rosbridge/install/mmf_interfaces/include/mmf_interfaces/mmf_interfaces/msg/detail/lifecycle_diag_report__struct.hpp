// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/LifecycleDiagReport.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'service_list'
#include "mmf_interfaces/msg/detail/service_status__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__LifecycleDiagReport __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__LifecycleDiagReport __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct LifecycleDiagReport_
{
  using Type = LifecycleDiagReport_<ContainerAllocator>;

  explicit LifecycleDiagReport_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->is_started = false;
      this->running_service_num = 0l;
      this->total_services_num = 0l;
    }
  }

  explicit LifecycleDiagReport_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->is_started = false;
      this->running_service_num = 0l;
      this->total_services_num = 0l;
    }
  }

  // field types and members
  using _is_started_type =
    bool;
  _is_started_type is_started;
  using _running_service_num_type =
    int32_t;
  _running_service_num_type running_service_num;
  using _total_services_num_type =
    int32_t;
  _total_services_num_type total_services_num;
  using _service_list_type =
    std::vector<mmf_interfaces::msg::ServiceStatus_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mmf_interfaces::msg::ServiceStatus_<ContainerAllocator>>>;
  _service_list_type service_list;

  // setters for named parameter idiom
  Type & set__is_started(
    const bool & _arg)
  {
    this->is_started = _arg;
    return *this;
  }
  Type & set__running_service_num(
    const int32_t & _arg)
  {
    this->running_service_num = _arg;
    return *this;
  }
  Type & set__total_services_num(
    const int32_t & _arg)
  {
    this->total_services_num = _arg;
    return *this;
  }
  Type & set__service_list(
    const std::vector<mmf_interfaces::msg::ServiceStatus_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mmf_interfaces::msg::ServiceStatus_<ContainerAllocator>>> & _arg)
  {
    this->service_list = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__LifecycleDiagReport
    std::shared_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__LifecycleDiagReport
    std::shared_ptr<mmf_interfaces::msg::LifecycleDiagReport_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const LifecycleDiagReport_ & other) const
  {
    if (this->is_started != other.is_started) {
      return false;
    }
    if (this->running_service_num != other.running_service_num) {
      return false;
    }
    if (this->total_services_num != other.total_services_num) {
      return false;
    }
    if (this->service_list != other.service_list) {
      return false;
    }
    return true;
  }
  bool operator!=(const LifecycleDiagReport_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct LifecycleDiagReport_

// alias to use template instance with default allocator
using LifecycleDiagReport =
  mmf_interfaces::msg::LifecycleDiagReport_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__LIFECYCLE_DIAG_REPORT__STRUCT_HPP_
