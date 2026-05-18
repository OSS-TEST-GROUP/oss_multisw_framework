// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/DiagnosticInfoListRequest.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/diagnostic_info_list_request__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
mmf_interfaces__msg__DiagnosticInfoListRequest__init(mmf_interfaces__msg__DiagnosticInfoListRequest * msg)
{
  if (!msg) {
    return false;
  }
  // structure_needs_at_least_one_member
  return true;
}

void
mmf_interfaces__msg__DiagnosticInfoListRequest__fini(mmf_interfaces__msg__DiagnosticInfoListRequest * msg)
{
  if (!msg) {
    return;
  }
  // structure_needs_at_least_one_member
}

bool
mmf_interfaces__msg__DiagnosticInfoListRequest__are_equal(const mmf_interfaces__msg__DiagnosticInfoListRequest * lhs, const mmf_interfaces__msg__DiagnosticInfoListRequest * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // structure_needs_at_least_one_member
  if (lhs->structure_needs_at_least_one_member != rhs->structure_needs_at_least_one_member) {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticInfoListRequest__copy(
  const mmf_interfaces__msg__DiagnosticInfoListRequest * input,
  mmf_interfaces__msg__DiagnosticInfoListRequest * output)
{
  if (!input || !output) {
    return false;
  }
  // structure_needs_at_least_one_member
  output->structure_needs_at_least_one_member = input->structure_needs_at_least_one_member;
  return true;
}

mmf_interfaces__msg__DiagnosticInfoListRequest *
mmf_interfaces__msg__DiagnosticInfoListRequest__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListRequest * msg = (mmf_interfaces__msg__DiagnosticInfoListRequest *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticInfoListRequest), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__DiagnosticInfoListRequest));
  bool success = mmf_interfaces__msg__DiagnosticInfoListRequest__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__DiagnosticInfoListRequest__destroy(mmf_interfaces__msg__DiagnosticInfoListRequest * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__DiagnosticInfoListRequest__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__init(mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListRequest * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__DiagnosticInfoListRequest *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__DiagnosticInfoListRequest), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__DiagnosticInfoListRequest__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__DiagnosticInfoListRequest__fini(&data[i - 1]);
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
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__fini(mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * array)
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
      mmf_interfaces__msg__DiagnosticInfoListRequest__fini(&array->data[i]);
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

mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence *
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * array = (mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__destroy(mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__are_equal(const mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * lhs, const mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticInfoListRequest__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence__copy(
  const mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * input,
  mmf_interfaces__msg__DiagnosticInfoListRequest__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__DiagnosticInfoListRequest);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__DiagnosticInfoListRequest * data =
      (mmf_interfaces__msg__DiagnosticInfoListRequest *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__DiagnosticInfoListRequest__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__DiagnosticInfoListRequest__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticInfoListRequest__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
