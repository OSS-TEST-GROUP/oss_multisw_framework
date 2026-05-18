// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/AdswServiceStatus.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/adsw_service_status__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `status_msg`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__AdswServiceStatus__init(mmf_interfaces__msg__AdswServiceStatus * msg)
{
  if (!msg) {
    return false;
  }
  // status_code
  // status_msg
  if (!rosidl_runtime_c__String__init(&msg->status_msg)) {
    mmf_interfaces__msg__AdswServiceStatus__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__AdswServiceStatus__fini(mmf_interfaces__msg__AdswServiceStatus * msg)
{
  if (!msg) {
    return;
  }
  // status_code
  // status_msg
  rosidl_runtime_c__String__fini(&msg->status_msg);
}

bool
mmf_interfaces__msg__AdswServiceStatus__are_equal(const mmf_interfaces__msg__AdswServiceStatus * lhs, const mmf_interfaces__msg__AdswServiceStatus * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // status_code
  if (lhs->status_code != rhs->status_code) {
    return false;
  }
  // status_msg
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->status_msg), &(rhs->status_msg)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__AdswServiceStatus__copy(
  const mmf_interfaces__msg__AdswServiceStatus * input,
  mmf_interfaces__msg__AdswServiceStatus * output)
{
  if (!input || !output) {
    return false;
  }
  // status_code
  output->status_code = input->status_code;
  // status_msg
  if (!rosidl_runtime_c__String__copy(
      &(input->status_msg), &(output->status_msg)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__AdswServiceStatus *
mmf_interfaces__msg__AdswServiceStatus__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__AdswServiceStatus * msg = (mmf_interfaces__msg__AdswServiceStatus *)allocator.allocate(sizeof(mmf_interfaces__msg__AdswServiceStatus), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__AdswServiceStatus));
  bool success = mmf_interfaces__msg__AdswServiceStatus__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__AdswServiceStatus__destroy(mmf_interfaces__msg__AdswServiceStatus * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__AdswServiceStatus__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__AdswServiceStatus__Sequence__init(mmf_interfaces__msg__AdswServiceStatus__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__AdswServiceStatus * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__AdswServiceStatus *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__AdswServiceStatus), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__AdswServiceStatus__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__AdswServiceStatus__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
mmf_interfaces__msg__AdswServiceStatus__Sequence__fini(mmf_interfaces__msg__AdswServiceStatus__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      mmf_interfaces__msg__AdswServiceStatus__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

mmf_interfaces__msg__AdswServiceStatus__Sequence *
mmf_interfaces__msg__AdswServiceStatus__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__AdswServiceStatus__Sequence * array = (mmf_interfaces__msg__AdswServiceStatus__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__AdswServiceStatus__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__AdswServiceStatus__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__AdswServiceStatus__Sequence__destroy(mmf_interfaces__msg__AdswServiceStatus__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__AdswServiceStatus__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__AdswServiceStatus__Sequence__are_equal(const mmf_interfaces__msg__AdswServiceStatus__Sequence * lhs, const mmf_interfaces__msg__AdswServiceStatus__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__AdswServiceStatus__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__AdswServiceStatus__Sequence__copy(
  const mmf_interfaces__msg__AdswServiceStatus__Sequence * input,
  mmf_interfaces__msg__AdswServiceStatus__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__AdswServiceStatus);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__AdswServiceStatus * data =
      (mmf_interfaces__msg__AdswServiceStatus *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__AdswServiceStatus__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__AdswServiceStatus__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__AdswServiceStatus__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
