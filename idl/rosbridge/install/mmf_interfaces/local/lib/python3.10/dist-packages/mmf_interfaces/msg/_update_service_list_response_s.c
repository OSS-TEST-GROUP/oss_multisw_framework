// generated from rosidl_generator_py/resource/_idl_support.c.em
// with input from mmf_interfaces:msg/UpdateServiceListResponse.idl
// generated code does not contain a copyright notice
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <stdbool.h>
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "numpy/ndarrayobject.h"
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif
#include "rosidl_runtime_c/visibility_control.h"
#include "mmf_interfaces/msg/detail/update_service_list_response__struct.h"
#include "mmf_interfaces/msg/detail/update_service_list_response__functions.h"


ROSIDL_GENERATOR_C_EXPORT
bool mmf_interfaces__msg__update_service_list_response__convert_from_py(PyObject * _pymsg, void * _ros_message)
{
  // check that the passed message is of the expected Python class
  {
    char full_classname_dest[75];
    {
      char * class_name = NULL;
      char * module_name = NULL;
      {
        PyObject * class_attr = PyObject_GetAttrString(_pymsg, "__class__");
        if (class_attr) {
          PyObject * name_attr = PyObject_GetAttrString(class_attr, "__name__");
          if (name_attr) {
            class_name = (char *)PyUnicode_1BYTE_DATA(name_attr);
            Py_DECREF(name_attr);
          }
          PyObject * module_attr = PyObject_GetAttrString(class_attr, "__module__");
          if (module_attr) {
            module_name = (char *)PyUnicode_1BYTE_DATA(module_attr);
            Py_DECREF(module_attr);
          }
          Py_DECREF(class_attr);
        }
      }
      if (!class_name || !module_name) {
        return false;
      }
      snprintf(full_classname_dest, sizeof(full_classname_dest), "%s.%s", module_name, class_name);
    }
    assert(strncmp("mmf_interfaces.msg._update_service_list_response.UpdateServiceListResponse", full_classname_dest, 74) == 0);
  }
  mmf_interfaces__msg__UpdateServiceListResponse * ros_message = _ros_message;
  {  // ret_code
    PyObject * field = PyObject_GetAttrString(_pymsg, "ret_code");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->ret_code = (int32_t)PyLong_AsLong(field);
    Py_DECREF(field);
  }

  return true;
}

ROSIDL_GENERATOR_C_EXPORT
PyObject * mmf_interfaces__msg__update_service_list_response__convert_to_py(void * raw_ros_message)
{
  /* NOTE(esteve): Call constructor of UpdateServiceListResponse */
  PyObject * _pymessage = NULL;
  {
    PyObject * pymessage_module = PyImport_ImportModule("mmf_interfaces.msg._update_service_list_response");
    assert(pymessage_module);
    PyObject * pymessage_class = PyObject_GetAttrString(pymessage_module, "UpdateServiceListResponse");
    assert(pymessage_class);
    Py_DECREF(pymessage_module);
    _pymessage = PyObject_CallObject(pymessage_class, NULL);
    Py_DECREF(pymessage_class);
    if (!_pymessage) {
      return NULL;
    }
  }
  mmf_interfaces__msg__UpdateServiceListResponse * ros_message = (mmf_interfaces__msg__UpdateServiceListResponse *)raw_ros_message;
  {  // ret_code
    PyObject * field = NULL;
    field = PyLong_FromLong(ros_message->ret_code);
    {
      int rc = PyObject_SetAttrString(_pymessage, "ret_code", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }

  // ownership of _pymessage is transferred to the caller
  return _pymessage;
}
