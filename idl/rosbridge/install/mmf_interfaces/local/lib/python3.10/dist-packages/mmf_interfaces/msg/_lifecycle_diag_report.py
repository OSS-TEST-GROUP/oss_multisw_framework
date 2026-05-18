# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mmf_interfaces:msg/LifecycleDiagReport.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_LifecycleDiagReport(type):
    """Metaclass of message 'LifecycleDiagReport'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('mmf_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'mmf_interfaces.msg.LifecycleDiagReport')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__lifecycle_diag_report
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__lifecycle_diag_report
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__lifecycle_diag_report
            cls._TYPE_SUPPORT = module.type_support_msg__msg__lifecycle_diag_report
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__lifecycle_diag_report

            from mmf_interfaces.msg import ServiceStatus
            if ServiceStatus.__class__._TYPE_SUPPORT is None:
                ServiceStatus.__class__.__import_type_support__()

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class LifecycleDiagReport(metaclass=Metaclass_LifecycleDiagReport):
    """Message class 'LifecycleDiagReport'."""

    __slots__ = [
        '_is_started',
        '_running_service_num',
        '_total_services_num',
        '_service_list',
    ]

    _fields_and_field_types = {
        'is_started': 'boolean',
        'running_service_num': 'int32',
        'total_services_num': 'int32',
        'service_list': 'sequence<mmf_interfaces/ServiceStatus>',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.NamespacedType(['mmf_interfaces', 'msg'], 'ServiceStatus')),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.is_started = kwargs.get('is_started', bool())
        self.running_service_num = kwargs.get('running_service_num', int())
        self.total_services_num = kwargs.get('total_services_num', int())
        self.service_list = kwargs.get('service_list', [])

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.is_started != other.is_started:
            return False
        if self.running_service_num != other.running_service_num:
            return False
        if self.total_services_num != other.total_services_num:
            return False
        if self.service_list != other.service_list:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def is_started(self):
        """Message field 'is_started'."""
        return self._is_started

    @is_started.setter
    def is_started(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'is_started' field must be of type 'bool'"
        self._is_started = value

    @builtins.property
    def running_service_num(self):
        """Message field 'running_service_num'."""
        return self._running_service_num

    @running_service_num.setter
    def running_service_num(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'running_service_num' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'running_service_num' field must be an integer in [-2147483648, 2147483647]"
        self._running_service_num = value

    @builtins.property
    def total_services_num(self):
        """Message field 'total_services_num'."""
        return self._total_services_num

    @total_services_num.setter
    def total_services_num(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'total_services_num' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'total_services_num' field must be an integer in [-2147483648, 2147483647]"
        self._total_services_num = value

    @builtins.property
    def service_list(self):
        """Message field 'service_list'."""
        return self._service_list

    @service_list.setter
    def service_list(self, value):
        if __debug__:
            from mmf_interfaces.msg import ServiceStatus
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 all(isinstance(v, ServiceStatus) for v in value) and
                 True), \
                "The 'service_list' field must be a set or sequence and each value of type 'ServiceStatus'"
        self._service_list = value
