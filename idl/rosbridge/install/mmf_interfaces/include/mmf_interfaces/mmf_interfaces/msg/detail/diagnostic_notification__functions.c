// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/DiagnosticNotification.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/diagnostic_notification__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `diag_name`
// Member `message`
// Member `source_message_name`
// Member `raw_data`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__DiagnosticNotification__init(mmf_interfaces__msg__DiagnosticNotification * msg)
{
  if (!msg) {
    return false;
  }
  // diag_code
  // diag_name
  if (!rosidl_runtime_c__String__init(&msg->diag_name)) {
    mmf_interfaces__msg__DiagnosticNotification__fini(msg);
    return false;
  }
  // status
  // message
  if (!rosidl_runtime_c__String__init(&msg->message)) {
    mmf_interfaces__msg__DiagnosticNotification__fini(msg);
    return false;
  }
  // source_message_name
  if (!rosidl_runtime_c__String__init(&msg->source_message_name)) {
    mmf_interfaces__msg__DiagnosticNotification__fini(msg);
    return false;
  }
  // raw_data
  if (!rosidl_runtime_c__String__init(&msg->raw_data)) {
    mmf_interfaces__msg__DiagnosticNotification__fini(msg);
    return false;
  }
  return true;
}

void
mmf_interfaces__msg__DiagnosticNotification__fini(mmf_interfaces__msg__DiagnosticNotification * msg)
{
  if (!msg) {
    return;
  }
  // diag_code
  // diag_name
  rosidl_runtime_c__String__fini(&msg->diag_name);
  // status
  // message
  rosidl_runtime_c__String__fini(&msg->message);
  // source_message_name
  rosidl_runtime_c__String__fini(&msg->source_message_name);
  // raw_data
  rosidl_runtime_c__String__fini(&msg->raw_data);
}

bool
mmf_interfaces__msg__DiagnosticNotification__are_equal(const mmf_interfaces__msg__DiagnosticNotification * lhs, const mmf_interfaces__msg__DiagnosticNotification * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // diag_code
  if (lhs->diag_code != rhs->diag_code) {
    return false;
  }
  // diag_name
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->diag_name), &(rhs->diag_name)))
  {
    return false;
  }
  // status
  if (lhs->status != rhs->status) {
    return false;
  }
  // message
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->message), &(rhs->message)))
  {
    return false;
  }
  // source_message_name
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->source_message_name), &(rhs->source_message_name)))
  {
    return false;
  }
  // raw_data
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->raw_data), &(rhs->raw_data)))
  {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticNotification__copy(
  const mmf_interfaces__msg__DiagnosticNotification * input,
  mmf_interfaces__msg__DiagnosticNotification * output)
{
  if (!input || !output) {
    return false;
  }
  // diag_code
  output->diag_code = input->diag_code;
  // diag_name
  if (!rosidl_runtime_c__String__copy(
      &(input->diag_name), &(output->diag_name)))
  {
    return false;
  }
  // status
  output->status = input->status;
  // message
  if (!rosidl_runtime_c__String__copy(
      &(input->message), &(output->message)))
  {
    return false;
  }
  // source_message_name
  if (!rosidl_runtime_c__String__copy(
      &(input->source_message_name), &(output->source_message_name)))
  {
    return false;
  }
  // raw_data
  if (!rosidl_runtime_c__String__copy(
      &(input->raw_data), &(output->raw_data)))
  {
    return false;
  }
  return true;
}

mmf_interfaces__msg__DiagnosticNotification *
mmf_interfaces__msg__DiagnosticNotification__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticNotification * msg = (mmf_interfaces__msg__DiagnosticNotification *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticNotification), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__DiagnosticNotification));
  bool success = mmf_interfaces__msg__DiagnosticNotification__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__DiagnosticNotification__destroy(mmf_interfaces__msg__DiagnosticNotification * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__DiagnosticNotification__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__DiagnosticNotification__Sequence__init(mmf_interfaces__msg__DiagnosticNotification__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticNotification * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__DiagnosticNotification *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__DiagnosticNotification), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__DiagnosticNotification__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__DiagnosticNotification__fini(&data[i - 1]);
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
mmf_interfaces__msg__DiagnosticNotification__Sequence__fini(mmf_interfaces__msg__DiagnosticNotification__Sequence * array)
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
      mmf_interfaces__msg__DiagnosticNotification__fini(&array->data[i]);
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

mmf_interfaces__msg__DiagnosticNotification__Sequence *
mmf_interfaces__msg__DiagnosticNotification__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DiagnosticNotification__Sequence * array = (mmf_interfaces__msg__DiagnosticNotification__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__DiagnosticNotification__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__DiagnosticNotification__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__DiagnosticNotification__Sequence__destroy(mmf_interfaces__msg__DiagnosticNotification__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__DiagnosticNotification__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__DiagnosticNotification__Sequence__are_equal(const mmf_interfaces__msg__DiagnosticNotification__Sequence * lhs, const mmf_interfaces__msg__DiagnosticNotification__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticNotification__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__DiagnosticNotification__Sequence__copy(
  const mmf_interfaces__msg__DiagnosticNotification__Sequence * input,
  mmf_interfaces__msg__DiagnosticNotification__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__DiagnosticNotification);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__DiagnosticNotification * data =
      (mmf_interfaces__msg__DiagnosticNotification *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__DiagnosticNotification__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__DiagnosticNotification__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__DiagnosticNotification__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
