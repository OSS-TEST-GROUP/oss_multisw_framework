// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/GetVehicleInfoResponse.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/get_vehicle_info_response__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `name`
// Member `value`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__GetVehicleInfoResponse__init(mmf_interfaces__msg__GetVehicleInfoResponse * msg)
{
  if (!msg) {
    return false;
  }
  // id
  // name
  if (!rosidl_runtime_c__String__init(&msg->name)) {
    mmf_interfaces__msg__GetVehicleInfoResponse__fini(msg);
    return false;
  }
  // value
  if (!rosidl_runtime_c__String__init(&msg->value)) {
    mmf_interfaces__msg__GetVehicleInfoResponse__fini(msg);
    return false;
  }
  // ret_code
  return true;
}

void
mmf_interfaces__msg__GetVehicleInfoResponse__fini(mmf_interfaces__msg__GetVehicleInfoResponse * msg)
{
  if (!msg) {
    return;
  }
  // id
  // name
  rosidl_runtime_c__String__fini(&msg->name);
  // value
  rosidl_runtime_c__String__fini(&msg->value);
  // ret_code
}

bool
mmf_interfaces__msg__GetVehicleInfoResponse__are_equal(const mmf_interfaces__msg__GetVehicleInfoResponse * lhs, const mmf_interfaces__msg__GetVehicleInfoResponse * rhs)
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
  // value
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->value), &(rhs->value)))
  {
    return false;
  }
  // ret_code
  if (lhs->ret_code != rhs->ret_code) {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__GetVehicleInfoResponse__copy(
  const mmf_interfaces__msg__GetVehicleInfoResponse * input,
  mmf_interfaces__msg__GetVehicleInfoResponse * output)
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
  // value
  if (!rosidl_runtime_c__String__copy(
      &(input->value), &(output->value)))
  {
    return false;
  }
  // ret_code
  output->ret_code = input->ret_code;
  return true;
}

mmf_interfaces__msg__GetVehicleInfoResponse *
mmf_interfaces__msg__GetVehicleInfoResponse__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfoResponse * msg = (mmf_interfaces__msg__GetVehicleInfoResponse *)allocator.allocate(sizeof(mmf_interfaces__msg__GetVehicleInfoResponse), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__GetVehicleInfoResponse));
  bool success = mmf_interfaces__msg__GetVehicleInfoResponse__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__GetVehicleInfoResponse__destroy(mmf_interfaces__msg__GetVehicleInfoResponse * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__GetVehicleInfoResponse__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfoResponse * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__GetVehicleInfoResponse *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__GetVehicleInfoResponse), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__GetVehicleInfoResponse__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__GetVehicleInfoResponse__fini(&data[i - 1]);
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
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array)
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
      mmf_interfaces__msg__GetVehicleInfoResponse__fini(&array->data[i]);
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

mmf_interfaces__msg__GetVehicleInfoResponse__Sequence *
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array = (mmf_interfaces__msg__GetVehicleInfoResponse__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__destroy(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__are_equal(const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * lhs, const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__GetVehicleInfoResponse__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__copy(
  const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * input,
  mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__GetVehicleInfoResponse);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__GetVehicleInfoResponse * data =
      (mmf_interfaces__msg__GetVehicleInfoResponse *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__GetVehicleInfoResponse__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__GetVehicleInfoResponse__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__GetVehicleInfoResponse__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
