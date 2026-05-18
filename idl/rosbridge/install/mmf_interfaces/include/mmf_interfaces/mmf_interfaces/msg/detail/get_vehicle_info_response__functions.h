// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from mmf_interfaces:msg/GetVehicleInfoResponse.idl
// generated code does not contain a copyright notice

#ifndef MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__FUNCTIONS_H_
#define MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "mmf_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "mmf_interfaces/msg/detail/get_vehicle_info_response__struct.h"

/// Initialize msg/GetVehicleInfoResponse message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * mmf_interfaces__msg__GetVehicleInfoResponse
 * )) before or use
 * mmf_interfaces__msg__GetVehicleInfoResponse__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__GetVehicleInfoResponse__init(mmf_interfaces__msg__GetVehicleInfoResponse * msg);

/// Finalize msg/GetVehicleInfoResponse message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__GetVehicleInfoResponse__fini(mmf_interfaces__msg__GetVehicleInfoResponse * msg);

/// Create msg/GetVehicleInfoResponse message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__GetVehicleInfoResponse *
mmf_interfaces__msg__GetVehicleInfoResponse__create();

/// Destroy msg/GetVehicleInfoResponse message.
/**
 * It calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__GetVehicleInfoResponse__destroy(mmf_interfaces__msg__GetVehicleInfoResponse * msg);

/// Check for msg/GetVehicleInfoResponse message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__GetVehicleInfoResponse__are_equal(const mmf_interfaces__msg__GetVehicleInfoResponse * lhs, const mmf_interfaces__msg__GetVehicleInfoResponse * rhs);

/// Copy a msg/GetVehicleInfoResponse message.
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
mmf_interfaces__msg__GetVehicleInfoResponse__copy(
  const mmf_interfaces__msg__GetVehicleInfoResponse * input,
  mmf_interfaces__msg__GetVehicleInfoResponse * output);

/// Initialize array of msg/GetVehicleInfoResponse messages.
/**
 * It allocates the memory for the number of elements and calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array, size_t size);

/// Finalize array of msg/GetVehicleInfoResponse messages.
/**
 * It calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array);

/// Create array of msg/GetVehicleInfoResponse messages.
/**
 * It allocates the memory for the array and calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence *
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__create(size_t size);

/// Destroy array of msg/GetVehicleInfoResponse messages.
/**
 * It calls
 * mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
void
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__destroy(mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * array);

/// Check for msg/GetVehicleInfoResponse message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mmf_interfaces
bool
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__are_equal(const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * lhs, const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * rhs);

/// Copy an array of msg/GetVehicleInfoResponse messages.
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
mmf_interfaces__msg__GetVehicleInfoResponse__Sequence__copy(
  const mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * input,
  mmf_interfaces__msg__GetVehicleInfoResponse__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // MMF_INTERFACES__MSG__DETAIL__GET_VEHICLE_INFO_RESPONSE__FUNCTIONS_H_
