// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mmf_interfaces:msg/DetectionReport.idl
// generated code does not contain a copyright notice
#include "mmf_interfaces/msg/detail/detection_report__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `client_id`
// Member `dds_topic`
// Member `rule`
#include "rosidl_runtime_c/string_functions.h"

bool
mmf_interfaces__msg__DetectionReport__init(mmf_interfaces__msg__DetectionReport * msg)
{
  if (!msg) {
    return false;
  }
  // client_id
  if (!rosidl_runtime_c__String__init(&msg->client_id)) {
    mmf_interfaces__msg__DetectionReport__fini(msg);
    return false;
  }
  // dds_topic
  if (!rosidl_runtime_c__String__init(&msg->dds_topic)) {
    mmf_interfaces__msg__DetectionReport__fini(msg);
    return false;
  }
  // rule
  if (!rosidl_runtime_c__String__init(&msg->rule)) {
    mmf_interfaces__msg__DetectionReport__fini(msg);
    return false;
  }
  // timestamp_ms
  return true;
}

void
mmf_interfaces__msg__DetectionReport__fini(mmf_interfaces__msg__DetectionReport * msg)
{
  if (!msg) {
    return;
  }
  // client_id
  rosidl_runtime_c__String__fini(&msg->client_id);
  // dds_topic
  rosidl_runtime_c__String__fini(&msg->dds_topic);
  // rule
  rosidl_runtime_c__String__fini(&msg->rule);
  // timestamp_ms
}

bool
mmf_interfaces__msg__DetectionReport__are_equal(const mmf_interfaces__msg__DetectionReport * lhs, const mmf_interfaces__msg__DetectionReport * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // client_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->client_id), &(rhs->client_id)))
  {
    return false;
  }
  // dds_topic
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->dds_topic), &(rhs->dds_topic)))
  {
    return false;
  }
  // rule
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->rule), &(rhs->rule)))
  {
    return false;
  }
  // timestamp_ms
  if (lhs->timestamp_ms != rhs->timestamp_ms) {
    return false;
  }
  return true;
}

bool
mmf_interfaces__msg__DetectionReport__copy(
  const mmf_interfaces__msg__DetectionReport * input,
  mmf_interfaces__msg__DetectionReport * output)
{
  if (!input || !output) {
    return false;
  }
  // client_id
  if (!rosidl_runtime_c__String__copy(
      &(input->client_id), &(output->client_id)))
  {
    return false;
  }
  // dds_topic
  if (!rosidl_runtime_c__String__copy(
      &(input->dds_topic), &(output->dds_topic)))
  {
    return false;
  }
  // rule
  if (!rosidl_runtime_c__String__copy(
      &(input->rule), &(output->rule)))
  {
    return false;
  }
  // timestamp_ms
  output->timestamp_ms = input->timestamp_ms;
  return true;
}

mmf_interfaces__msg__DetectionReport *
mmf_interfaces__msg__DetectionReport__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DetectionReport * msg = (mmf_interfaces__msg__DetectionReport *)allocator.allocate(sizeof(mmf_interfaces__msg__DetectionReport), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mmf_interfaces__msg__DetectionReport));
  bool success = mmf_interfaces__msg__DetectionReport__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mmf_interfaces__msg__DetectionReport__destroy(mmf_interfaces__msg__DetectionReport * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mmf_interfaces__msg__DetectionReport__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mmf_interfaces__msg__DetectionReport__Sequence__init(mmf_interfaces__msg__DetectionReport__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DetectionReport * data = NULL;

  if (size) {
    data = (mmf_interfaces__msg__DetectionReport *)allocator.zero_allocate(size, sizeof(mmf_interfaces__msg__DetectionReport), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mmf_interfaces__msg__DetectionReport__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mmf_interfaces__msg__DetectionReport__fini(&data[i - 1]);
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
mmf_interfaces__msg__DetectionReport__Sequence__fini(mmf_interfaces__msg__DetectionReport__Sequence * array)
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
      mmf_interfaces__msg__DetectionReport__fini(&array->data[i]);
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

mmf_interfaces__msg__DetectionReport__Sequence *
mmf_interfaces__msg__DetectionReport__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mmf_interfaces__msg__DetectionReport__Sequence * array = (mmf_interfaces__msg__DetectionReport__Sequence *)allocator.allocate(sizeof(mmf_interfaces__msg__DetectionReport__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mmf_interfaces__msg__DetectionReport__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mmf_interfaces__msg__DetectionReport__Sequence__destroy(mmf_interfaces__msg__DetectionReport__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mmf_interfaces__msg__DetectionReport__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mmf_interfaces__msg__DetectionReport__Sequence__are_equal(const mmf_interfaces__msg__DetectionReport__Sequence * lhs, const mmf_interfaces__msg__DetectionReport__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mmf_interfaces__msg__DetectionReport__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mmf_interfaces__msg__DetectionReport__Sequence__copy(
  const mmf_interfaces__msg__DetectionReport__Sequence * input,
  mmf_interfaces__msg__DetectionReport__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mmf_interfaces__msg__DetectionReport);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mmf_interfaces__msg__DetectionReport * data =
      (mmf_interfaces__msg__DetectionReport *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mmf_interfaces__msg__DetectionReport__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mmf_interfaces__msg__DetectionReport__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mmf_interfaces__msg__DetectionReport__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
