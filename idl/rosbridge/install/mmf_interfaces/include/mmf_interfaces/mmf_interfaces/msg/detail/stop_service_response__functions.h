// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from mmf_interfaces:msg/StopServiceResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__FUNCTIONS_H_
#define MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "mmf_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "mmf_interfaces/msg/detail/stop_service_response__struct.h"

/// Initialize msg/StopServiceResponse message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * mmf_interfaces__msg__StopServiceResponse
 * )) before or use
 * mmf_interfaces__msg__StopServiceResponse__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__StopServiceResponse__init(mmf_interfaces__msg__StopServiceResponse * msg);

/// Finalize msg/StopServiceResponse message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__StopServiceResponse__fini(mmf_interfaces__msg__StopServiceResponse * msg);

/// Create msg/StopServiceResponse message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * mmf_interfaces__msg__StopServiceResponse__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__StopServiceResponse *
mmf_interfaces__msg__StopServiceResponse__create();

/// Destroy msg/StopServiceResponse message.
/**
 * It calls
 * mmf_interfaces__msg__StopServiceResponse__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__StopServiceResponse__destroy(mmf_interfaces__msg__StopServiceResponse * msg);

/// Check for msg/StopServiceResponse message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__StopServiceResponse__are_equal(const mmf_interfaces__msg__StopServiceResponse * lhs, const mmf_interfaces__msg__StopServiceResponse * rhs);

/// Copy a msg/StopServiceResponse message.
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
mmf_interfaces__msg__StopServiceResponse__copy(
  const mmf_interfaces__msg__StopServiceResponse * input,
  mmf_interfaces__msg__StopServiceResponse * output);

/// Initialize array of msg/StopServiceResponse messages.
/**
 * It allocates the memory for the number of elements and calls
 * mmf_interfaces__msg__StopServiceResponse__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__StopServiceResponse__Sequence__init(mmf_interfaces__msg__StopServiceResponse__Sequence * array, size_t size);

/// Finalize array of msg/StopServiceResponse messages.
/**
 * It calls
 * mmf_interfaces__msg__StopServiceResponse__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__StopServiceResponse__Sequence__fini(mmf_interfaces__msg__StopServiceResponse__Sequence * array);

/// Create array of msg/StopServiceResponse messages.
/**
 * It allocates the memory for the array and calls
 * mmf_interfaces__msg__StopServiceResponse__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__StopServiceResponse__Sequence *
mmf_interfaces__msg__StopServiceResponse__Sequence__create(size_t size);

/// Destroy array of msg/StopServiceResponse messages.
/**
 * It calls
 * mmf_interfaces__msg__StopServiceResponse__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__StopServiceResponse__Sequence__destroy(mmf_interfaces__msg__StopServiceResponse__Sequence * array);

/// Check for msg/StopServiceResponse message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__StopServiceResponse__Sequence__are_equal(const mmf_interfaces__msg__StopServiceResponse__Sequence * lhs, const mmf_interfaces__msg__StopServiceResponse__Sequence * rhs);

/// Copy an array of msg/StopServiceResponse messages.
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
mmf_interfaces__msg__StopServiceResponse__Sequence__copy(
  const mmf_interfaces__msg__StopServiceResponse__Sequence * input,
  mmf_interfaces__msg__StopServiceResponse__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__STOP_SERVICE_RESPONSE__FUNCTIONS_H_
