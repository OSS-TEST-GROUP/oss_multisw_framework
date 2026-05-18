// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/DiagnosticInfoListResponse.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/diagnostic_info_list_response__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `diag_codes`
#include "rosidl_runtime_c/primitives_sequence_functions.h"
// Member `diag_names`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__DiagnosticInfoListResponse__init(mmf_interfaces__msg__DiagnosticInfoListResponse * msg)
{
  if (!msg) {
    return false;
  }
  // ret_code
  // diag_codes
  if (!rosidl_runtime_c__int32__Sequence__init(&msg->diag_codes, 0)) {
    mmf_interfaces__msg__DiagnosticInfoListResponse__fini(msg);
    return false;
  }
  // diag_names
  if (!rosidl_runtime_c__String__Sequence__init(&msg->diag_names, 0)) {
    mmf_interfaces__msg__DiagnosticInfoListResponse__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__DiagnosticInfoListResponse__fini(mmf_interfaces__msg__DiagnosticInfoListResponse * msg)
{
  if (!msg) {
    return;
  }
  // ret_code
  // diag_codes
  rosidl_runtime_c__int32__Sequence__fini(&msg->diag_codes);
  // diag_names
  rosidl_runtime_c__String__Sequence__fini(&msg->diag_names);
}

bool
mmf_interfaces__msg__DiagnosticInfoListResponse__are_equal(const mmf_interfaces__msg__DiagnosticInfoListResponse * lhs, const mmf_interfaces__msg__DiagnosticInfoListResponse * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // ret_code
  if (lhs->ret_code != rhs->ret_code) {
    return false;
  }
  // diag_codes
  if (!rosidl_runtime_c__int32__Sequence__are_equal(
      &(lhs->diag_codes), &(rhs->diag_codes)))
  {
    return false;
  }
  // diag_names
  if (!rosidl_runtime_c__String__Sequence__are_equal(
      &(lhs->diag_names), &(rhs->diag_names)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticInfoListResponse__copy(
  const mmf_interfaces__msg__DiagnosticInfoListResponse * input,
  mmf_interfaces__msg__DiagnosticInfoListResponse * output)
{
  if (!input || !output) {
    return false;
  }
  // ret_code
  output->ret_code = input->ret_code;
  // diag_codes
  if (!rosidl_runtime_c__int32__Sequence__copy(
      &(input->diag_codes), &(output->diag_codes)))
  {
    return false;
  }
  // diag_names
  if (!rosidl_runtime_c__String__Sequence__copy(
      &(input->diag_names), &(output->diag_names)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__DiagnosticInfoListResponse *
mmf_interfaces__msg__DiagnosticInfoListResponse__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListResponse * msg = (mmf_interfaces__msg__DiagnosticInfoListResponse *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticInfoListResponse), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__DiagnosticInfoListResponse));
  bool success = mmf_interfaces__msg__DiagnosticInfoListResponse__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__DiagnosticInfoListResponse__destroy(mmf_interfaces__msg__DiagnosticInfoListResponse * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__DiagnosticInfoListResponse__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__init(mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListResponse * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__DiagnosticInfoListResponse *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__DiagnosticInfoListResponse), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__DiagnosticInfoListResponse__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__DiagnosticInfoListResponse__fini(&data[i - 1]);
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
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__fini(mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * array)
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
      mmf_interfaces__msg__DiagnosticInfoListResponse__fini(&array->data[i]);
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

mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence *
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * array = (mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__destroy(mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__are_equal(const mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * lhs, const mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticInfoListResponse__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence__copy(
  const mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * input,
  mmf_interfaces__msg__DiagnosticInfoListResponse__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__DiagnosticInfoListResponse);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__DiagnosticInfoListResponse * data =
      (mmf_interfaces__msg__DiagnosticInfoListResponse *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__DiagnosticInfoListResponse__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__DiagnosticInfoListResponse__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticInfoListResponse__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
