// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from mmf_interfaces:msg/NotifyServiceState.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__FUNCTIONS_H_
#define MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "mmf_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "mmf_interfaces/msg/detail/notify_service_state__struct.h"

/// Initialize msg/NotifyServiceState message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * mmf_interfaces__msg__NotifyServiceState
 * )) before or use
 * mmf_interfaces__msg__NotifyServiceState__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__init(mmf_interfaces__msg__NotifyServiceState * msg);

/// Finalize msg/NotifyServiceState message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__NotifyServiceState__fini(mmf_interfaces__msg__NotifyServiceState * msg);

/// Create msg/NotifyServiceState message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * mmf_interfaces__msg__NotifyServiceState__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__NotifyServiceState *
mmf_interfaces__msg__NotifyServiceState__create();

/// Destroy msg/NotifyServiceState message.
/**
 * It calls
 * mmf_interfaces__msg__NotifyServiceState__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__NotifyServiceState__destroy(mmf_interfaces__msg__NotifyServiceState * msg);

/// Check for msg/NotifyServiceState message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__are_equal(const mmf_interfaces__msg__NotifyServiceState * lhs, const mmf_interfaces__msg__NotifyServiceState * rhs);

/// Copy a msg/NotifyServiceState message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__copy(
  const mmf_interfaces__msg__NotifyServiceState * input,
  mmf_interfaces__msg__NotifyServiceState * output);

/// Initialize array of msg/NotifyServiceState messages.
/**
 * It allocates the memory for the number of elements and calls
 * mmf_interfaces__msg__NotifyServiceState__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__Sequence__init(mmf_interfaces__msg__NotifyServiceState__Sequence * array, size_t size);

/// Finalize array of msg/NotifyServiceState messages.
/**
 * It calls
 * mmf_interfaces__msg__NotifyServiceState__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__NotifyServiceState__Sequence__fini(mmf_interfaces__msg__NotifyServiceState__Sequence * array);

/// Create array of msg/NotifyServiceState messages.
/**
 * It allocates the memory for the array and calls
 * mmf_interfaces__msg__NotifyServiceState__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__NotifyServiceState__Sequence *
mmf_interfaces__msg__NotifyServiceState__Sequence__create(size_t size);

/// Destroy array of msg/NotifyServiceState messages.
/**
 * It calls
 * mmf_interfaces__msg__NotifyServiceState__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__NotifyServiceState__Sequence__destroy(mmf_interfaces__msg__NotifyServiceState__Sequence * array);

/// Check for msg/NotifyServiceState message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__Sequence__are_equal(const mmf_interfaces__msg__NotifyServiceState__Sequence * lhs, const mmf_interfaces__msg__NotifyServiceState__Sequence * rhs);

/// Copy an array of msg/NotifyServiceState messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__NotifyServiceState__Sequence__copy(
  const mmf_interfaces__msg__NotifyServiceState__Sequence * input,
  mmf_interfaces__msg__NotifyServiceState__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__NOTIFY_SERVICE_STATE__FUNCTIONS_H_
