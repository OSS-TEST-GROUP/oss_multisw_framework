// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/UpdateServiceListResponse.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/update_service_list_response__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
mmf_interfaces__msg__UpdateServiceListResponse__init(mmf_interfaces__msg__UpdateServiceListResponse * msg)
{
  if (!msg) {
    return false;
  }
  // ret_code
  return true;
}

void
mmf_interfaces__msg__UpdateServiceListResponse__fini(mmf_interfaces__msg__UpdateServiceListResponse * msg)
{
  if (!msg) {
    return;
  }
  // ret_code
}

bool
mmf_interfaces__msg__UpdateServiceListResponse__are_equal(const mmf_interfaces__msg__UpdateServiceListResponse * lhs, const mmf_interfaces__msg__UpdateServiceListResponse * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // ret_code
  if (lhs->ret_code != rhs->ret_code) {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__UpdateServiceListResponse__copy(
  const mmf_interfaces__msg__UpdateServiceListResponse * input,
  mmf_interfaces__msg__UpdateServiceListResponse * output)
{
  if (!input || !output) {
    return false;
  }
  // ret_code
  output->ret_code = input->ret_code;
  return true;
}

mmf_interfaces__msg__UpdateServiceListResponse *
mmf_interfaces__msg__UpdateServiceListResponse__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__UpdateServiceListResponse * msg = (mmf_interfaces__msg__UpdateServiceListResponse *)allocator.allocate(sizeof(mmf_interfaces__msg__UpdateServiceListResponse), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__UpdateServiceListResponse));
  bool success = mmf_interfaces__msg__UpdateServiceListResponse__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__UpdateServiceListResponse__destroy(mmf_interfaces__msg__UpdateServiceListResponse * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__UpdateServiceListResponse__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__init(mmf_interfaces__msg__UpdateServiceListResponse__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__UpdateServiceListResponse * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__UpdateServiceListResponse *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__UpdateServiceListResponse), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__UpdateServiceListResponse__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__UpdateServiceListResponse__fini(&data[i - 1]);
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
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__fini(mmf_interfaces__msg__UpdateServiceListResponse__Sequence * array)
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
      mmf_interfaces__msg__UpdateServiceListResponse__fini(&array->data[i]);
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

mmf_interfaces__msg__UpdateServiceListResponse__Sequence *
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__UpdateServiceListResponse__Sequence * array = (mmf_interfaces__msg__UpdateServiceListResponse__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__UpdateServiceListResponse__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__UpdateServiceListResponse__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__destroy(mmf_interfaces__msg__UpdateServiceListResponse__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__UpdateServiceListResponse__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__are_equal(const mmf_interfaces__msg__UpdateServiceListResponse__Sequence * lhs, const mmf_interfaces__msg__UpdateServiceListResponse__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__UpdateServiceListResponse__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__UpdateServiceListResponse__Sequence__copy(
  const mmf_interfaces__msg__UpdateServiceListResponse__Sequence * input,
  mmf_interfaces__msg__UpdateServiceListResponse__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__UpdateServiceListResponse);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__UpdateServiceListResponse * data =
      (mmf_interfaces__msg__UpdateServiceListResponse *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__UpdateServiceListResponse__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__UpdateServiceListResponse__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__UpdateServiceListResponse__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
