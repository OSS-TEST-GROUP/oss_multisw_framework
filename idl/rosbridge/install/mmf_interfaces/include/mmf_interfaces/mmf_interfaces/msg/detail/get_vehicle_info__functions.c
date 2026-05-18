// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/GetVehicleInfo.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/get_vehicle_info__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `name`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__GetVehicleInfo__init(mmf_interfaces__msg__GetVehicleInfo * msg)
{
  if (!msg) {
    return false;
  }
  // id
  // name
  if (!rosidl_runtime_c__String__init(&msg->name)) {
    mmf_interfaces__msg__GetVehicleInfo__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__GetVehicleInfo__fini(mmf_interfaces__msg__GetVehicleInfo * msg)
{
  if (!msg) {
    return;
  }
  // id
  // name
  rosidl_runtime_c__String__fini(&msg->name);
}

bool
mmf_interfaces__msg__GetVehicleInfo__are_equal(const mmf_interfaces__msg__GetVehicleInfo * lhs, const mmf_interfaces__msg__GetVehicleInfo * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // id
  if (lhs->id != rhs->id) {
    return false;
  }
  // name
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->name), &(rhs->name)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__GetVehicleInfo__copy(
  const mmf_interfaces__msg__GetVehicleInfo * input,
  mmf_interfaces__msg__GetVehicleInfo * output)
{
  if (!input || !output) {
    return false;
  }
  // id
  output->id = input->id;
  // name
  if (!rosidl_runtime_c__String__copy(
      &(input->name), &(output->name)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__GetVehicleInfo *
mmf_interfaces__msg__GetVehicleInfo__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfo * msg = (mmf_interfaces__msg__GetVehicleInfo *)allocator.allocate(sizeof(mmf_interfaces__msg__GetVehicleInfo), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__GetVehicleInfo));
  bool success = mmf_interfaces__msg__GetVehicleInfo__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__GetVehicleInfo__destroy(mmf_interfaces__msg__GetVehicleInfo * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__GetVehicleInfo__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__GetVehicleInfo__Sequence__init(mmf_interfaces__msg__GetVehicleInfo__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfo * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__GetVehicleInfo *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__GetVehicleInfo), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__GetVehicleInfo__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__GetVehicleInfo__fini(&data[i - 1]);
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
mmf_interfaces__msg__GetVehicleInfo__Sequence__fini(mmf_interfaces__msg__GetVehicleInfo__Sequence * array)
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
      mmf_interfaces__msg__GetVehicleInfo__fini(&array->data[i]);
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

mmf_interfaces__msg__GetVehicleInfo__Sequence *
mmf_interfaces__msg__GetVehicleInfo__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfo__Sequence * array = (mmf_interfaces__msg__GetVehicleInfo__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__GetVehicleInfo__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__GetVehicleInfo__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__GetVehicleInfo__Sequence__destroy(mmf_interfaces__msg__GetVehicleInfo__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__GetVehicleInfo__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__GetVehicleInfo__Sequence__are_equal(const mmf_interfaces__msg__GetVehicleInfo__Sequence * lhs, const mmf_interfaces__msg__GetVehicleInfo__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__GetVehicleInfo__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__GetVehicleInfo__Sequence__copy(
  const mmf_interfaces__msg__GetVehicleInfo__Sequence * input,
  mmf_interfaces__msg__GetVehicleInfo__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__GetVehicleInfo);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__GetVehicleInfo * data =
      (mmf_interfaces__msg__GetVehicleInfo *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__GetVehicleInfo__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__GetVehicleInfo__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__GetVehicleInfo__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
