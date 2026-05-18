// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mmf_interfaces:msg/UpdateServiceListRequest.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__STRUCT_HPP_
#define MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__STRUCT_HPP_

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
#include "mmf_interfaces/msg/detail/lifecycle_service__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__mmf_interfaces__msg__UpdateServiceListRequest __attribute__((deprecated))
#else
# define DEPRECATED__mmf_interfaces__msg__UpdateServiceListRequest __declspec(deprecated)
#endif

namespace mmf_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct UpdateServiceListRequest_
{
  using Type = UpdateServiceListRequest_<ContainerAllocator>;

  explicit UpdateServiceListRequest_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_init;
  }

  explicit UpdateServiceListRequest_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_init;
    (void)_alloc;
  }

  // field types and members
  using _service_list_type =
    std::vector<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>>;
  _service_list_type service_list;

  // setters for named parameter idiom
  Type & set__service_list(
    const std::vector<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mmf_interfaces::msg::LifecycleService_<ContainerAllocator>>> & _arg)
  {
    this->service_list = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> *;
  using ConstRawPtr =
    const mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mmf_interfaces__msg__UpdateServiceListRequest
    std::shared_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mmf_interfaces__msg__UpdateServiceListRequest
    std::shared_ptr<mmf_interfaces::msg::UpdateServiceListRequest_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const UpdateServiceListRequest_ & other) const
  {
    if (this->service_list != other.service_list) {
      return false;
    }
    return true;
  }
  bool operator!=(const UpdateServiceListRequest_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct UpdateServiceListRequest_

// alias to use template instance with default allocator
using UpdateServiceListRequest =
  mmf_interfaces::msg::UpdateServiceListRequest_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mmf_interfaces

#endif  // MMF_INTERFACES__MSG__DETAIL__UPDATE_SERVICE_LIST_REQUEST__STRUCT_HPP_
