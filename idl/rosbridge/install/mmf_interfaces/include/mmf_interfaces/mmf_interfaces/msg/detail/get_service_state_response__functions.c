// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/GetServiceStateResponse.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/get_service_state_response__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `service_list`
#include "rosidl_runtime_c/string_functions.h"
// Member `service_state_list`
#include "rosidl_runtime_c/primitives_sequence_functions.h"

bool
mmf_interfaces__msg__GetServiceStateResponse__init(mmf_interfaces__msg__GetServiceStateResponse * msg)
{
  if (!msg) {
    return false;
  }
  // service_list
  if (!rosidl_runtime_c__String__Sequence__init(&msg->service_list, 0)) {
    mmf_interfaces__msg__GetServiceStateResponse__fini(msg);
    return false;
  }
  // service_state_list
  if (!rosidl_runtime_c__int32__Sequence__init(&msg->service_state_list, 0)) {
    mmf_interfaces__msg__GetServiceStateResponse__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__GetServiceStateResponse__fini(mmf_interfaces__msg__GetServiceStateResponse * msg)
{
  if (!msg) {
    return;
  }
  // service_list
  rosidl_runtime_c__String__Sequence__fini(&msg->service_list);
  // service_state_list
  rosidl_runtime_c__int32__Sequence__fini(&msg->service_state_list);
}

bool
mmf_interfaces__msg__GetServiceStateResponse__are_equal(const mmf_interfaces__msg__GetServiceStateResponse * lhs, const mmf_interfaces__msg__GetServiceStateResponse * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // service_list
  if (!rosidl_runtime_c__String__Sequence__are_equal(
      &(lhs->service_list), &(rhs->service_list)))
  {
    return false;
  }
  // service_state_list
  if (!rosidl_runtime_c__int32__Sequence__are_equal(
      &(lhs->service_state_list), &(rhs->service_state_list)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__GetServiceStateResponse__copy(
  const mmf_interfaces__msg__GetServiceStateResponse * input,
  mmf_interfaces__msg__GetServiceStateResponse * output)
{
  if (!input || !output) {
    return false;
  }
  // service_list
  if (!rosidl_runtime_c__String__Sequence__copy(
      &(input->service_list), &(output->service_list)))
  {
    return false;
  }
  // service_state_list
  if (!rosidl_runtime_c__int32__Sequence__copy(
      &(input->service_state_list), &(output->service_state_list)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__GetServiceStateResponse *
mmf_interfaces__msg__GetServiceStateResponse__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetServiceStateResponse * msg = (mmf_interfaces__msg__GetServiceStateResponse *)allocator.allocate(sizeof(mmf_interfaces__msg__GetServiceStateResponse), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__GetServiceStateResponse));
  bool success = mmf_interfaces__msg__GetServiceStateResponse__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__GetServiceStateResponse__destroy(mmf_interfaces__msg__GetServiceStateResponse * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__GetServiceStateResponse__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__GetServiceStateResponse__Sequence__init(mmf_interfaces__msg__GetServiceStateResponse__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetServiceStateResponse * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__GetServiceStateResponse *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__GetServiceStateResponse), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__GetServiceStateResponse__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__GetServiceStateResponse__fini(&data[i - 1]);
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
mmf_interfaces__msg__GetServiceStateResponse__Sequence__fini(mmf_interfaces__msg__GetServiceStateResponse__Sequence * array)
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
      mmf_interfaces__msg__GetServiceStateResponse__fini(&array->data[i]);
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

mmf_interfaces__msg__GetServiceStateResponse__Sequence *
mmf_interfaces__msg__GetServiceStateResponse__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetServiceStateResponse__Sequence * array = (mmf_interfaces__msg__GetServiceStateResponse__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__GetServiceStateResponse__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__GetServiceStateResponse__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__GetServiceStateResponse__Sequence__destroy(mmf_interfaces__msg__GetServiceStateResponse__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__GetServiceStateResponse__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__GetServiceStateResponse__Sequence__are_equal(const mmf_interfaces__msg__GetServiceStateResponse__Sequence * lhs, const mmf_interfaces__msg__GetServiceStateResponse__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__GetServiceStateResponse__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__GetServiceStateResponse__Sequence__copy(
  const mmf_interfaces__msg__GetServiceStateResponse__Sequence * input,
  mmf_interfaces__msg__GetServiceStateResponse__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__GetServiceStateResponse);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__GetServiceStateResponse * data =
      (mmf_interfaces__msg__GetServiceStateResponse *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__GetServiceStateResponse__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__GetServiceStateResponse__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__GetServiceStateResponse__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
