// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/ServiceState.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/service_state__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
mmf_interfaces__msg__ServiceState__init(mmf_interfaces__msg__ServiceState * msg)
{
  if (!msg) {
    return false;
  }
  // value
  return true;
}

void
mmf_interfaces__msg__ServiceState__fini(mmf_interfaces__msg__ServiceState * msg)
{
  if (!msg) {
    return;
  }
  // value
}

bool
mmf_interfaces__msg__ServiceState__are_equal(const mmf_interfaces__msg__ServiceState * lhs, const mmf_interfaces__msg__ServiceState * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // value
  if (lhs->value != rhs->value) {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__ServiceState__copy(
  const mmf_interfaces__msg__ServiceState * input,
  mmf_interfaces__msg__ServiceState * output)
{
  if (!input || !output) {
    return false;
  }
  // value
  output->value = input->value;
  return true;
}

mmf_interfaces__msg__ServiceState *
mmf_interfaces__msg__ServiceState__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__ServiceState * msg = (mmf_interfaces__msg__ServiceState *)allocator.allocate(sizeof(mmf_interfaces__msg__ServiceState), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__ServiceState));
  bool success = mmf_interfaces__msg__ServiceState__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__ServiceState__destroy(mmf_interfaces__msg__ServiceState * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__ServiceState__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__ServiceState__Sequence__init(mmf_interfaces__msg__ServiceState__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__ServiceState * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__ServiceState *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__ServiceState), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__ServiceState__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__ServiceState__fini(&data[i - 1]);
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
mmf_interfaces__msg__ServiceState__Sequence__fini(mmf_interfaces__msg__ServiceState__Sequence * array)
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
      mmf_interfaces__msg__ServiceState__fini(&array->data[i]);
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

mmf_interfaces__msg__ServiceState__Sequence *
mmf_interfaces__msg__ServiceState__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__ServiceState__Sequence * array = (mmf_interfaces__msg__ServiceState__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__ServiceState__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__ServiceState__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__ServiceState__Sequence__destroy(mmf_interfaces__msg__ServiceState__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__ServiceState__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__ServiceState__Sequence__are_equal(const mmf_interfaces__msg__ServiceState__Sequence * lhs, const mmf_interfaces__msg__ServiceState__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__ServiceState__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__ServiceState__Sequence__copy(
  const mmf_interfaces__msg__ServiceState__Sequence * input,
  mmf_interfaces__msg__ServiceState__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__ServiceState);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__ServiceState * data =
      (mmf_interfaces__msg__ServiceState *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__ServiceState__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__ServiceState__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__ServiceState__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
