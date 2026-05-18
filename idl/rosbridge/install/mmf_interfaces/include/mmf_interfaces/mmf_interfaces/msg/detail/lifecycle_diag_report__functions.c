// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/LifecycleDiagReport.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/lifecycle_diag_report__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `service_list`
#include "mmf_interfaces/msg/detail/service_status__functions.h"

bool
mmf_interfaces__msg__LifecycleDiagReport__init(mmf_interfaces__msg__LifecycleDiagReport * msg)
{
  if (!msg) {
    return false;
  }
  // is_started
  // running_service_num
  // total_services_num
  // service_list
  if (!mmf_interfaces__msg__ServiceStatus__Sequence__init(&msg->service_list, 0)) {
    mmf_interfaces__msg__LifecycleDiagReport__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__LifecycleDiagReport__fini(mmf_interfaces__msg__LifecycleDiagReport * msg)
{
  if (!msg) {
    return;
  }
  // is_started
  // running_service_num
  // total_services_num
  // service_list
  mmf_interfaces__msg__ServiceStatus__Sequence__fini(&msg->service_list);
}

bool
mmf_interfaces__msg__LifecycleDiagReport__are_equal(const mmf_interfaces__msg__LifecycleDiagReport * lhs, const mmf_interfaces__msg__LifecycleDiagReport * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // is_started
  if (lhs->is_started != rhs->is_started) {
    return false;
  }
  // running_service_num
  if (lhs->running_service_num != rhs->running_service_num) {
    return false;
  }
  // total_services_num
  if (lhs->total_services_num != rhs->total_services_num) {
    return false;
  }
  // service_list
  if (!mmf_interfaces__msg__ServiceStatus__Sequence__are_equal(
      &(lhs->service_list), &(rhs->service_list)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__LifecycleDiagReport__copy(
  const mmf_interfaces__msg__LifecycleDiagReport * input,
  mmf_interfaces__msg__LifecycleDiagReport * output)
{
  if (!input || !output) {
    return false;
  }
  // is_started
  output->is_started = input->is_started;
  // running_service_num
  output->running_service_num = input->running_service_num;
  // total_services_num
  output->total_services_num = input->total_services_num;
  // service_list
  if (!mmf_interfaces__msg__ServiceStatus__Sequence__copy(
      &(input->service_list), &(output->service_list)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__LifecycleDiagReport *
mmf_interfaces__msg__LifecycleDiagReport__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__LifecycleDiagReport * msg = (mmf_interfaces__msg__LifecycleDiagReport *)allocator.allocate(sizeof(mmf_interfaces__msg__LifecycleDiagReport), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__LifecycleDiagReport));
  bool success = mmf_interfaces__msg__LifecycleDiagReport__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__LifecycleDiagReport__destroy(mmf_interfaces__msg__LifecycleDiagReport * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__LifecycleDiagReport__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__LifecycleDiagReport__Sequence__init(mmf_interfaces__msg__LifecycleDiagReport__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__LifecycleDiagReport * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__LifecycleDiagReport *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__LifecycleDiagReport), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__LifecycleDiagReport__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__LifecycleDiagReport__fini(&data[i - 1]);
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
mmf_interfaces__msg__LifecycleDiagReport__Sequence__fini(mmf_interfaces__msg__LifecycleDiagReport__Sequence * array)
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
      mmf_interfaces__msg__LifecycleDiagReport__fini(&array->data[i]);
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

mmf_interfaces__msg__LifecycleDiagReport__Sequence *
mmf_interfaces__msg__LifecycleDiagReport__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__LifecycleDiagReport__Sequence * array = (mmf_interfaces__msg__LifecycleDiagReport__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__LifecycleDiagReport__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__LifecycleDiagReport__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__LifecycleDiagReport__Sequence__destroy(mmf_interfaces__msg__LifecycleDiagReport__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__LifecycleDiagReport__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__LifecycleDiagReport__Sequence__are_equal(const mmf_interfaces__msg__LifecycleDiagReport__Sequence * lhs, const mmf_interfaces__msg__LifecycleDiagReport__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__LifecycleDiagReport__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__LifecycleDiagReport__Sequence__copy(
  const mmf_interfaces__msg__LifecycleDiagReport__Sequence * input,
  mmf_interfaces__msg__LifecycleDiagReport__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__LifecycleDiagReport);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__LifecycleDiagReport * data =
      (mmf_interfaces__msg__LifecycleDiagReport *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__LifecycleDiagReport__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__LifecycleDiagReport__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__LifecycleDiagReport__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
