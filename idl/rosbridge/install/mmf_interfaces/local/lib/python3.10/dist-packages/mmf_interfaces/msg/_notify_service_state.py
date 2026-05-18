# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mmf_interfaces:msg/NotifyServiceState.idl
# generated code does not contain a copyright notice


# Import statements for member types

# Member 'service_state_list'
import array  # noqa: E402, I100

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_NotifyServiceState(type):
    """Metaclass of message 'NotifyServiceState'."""

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
                'mmf_interfaces.msg.NotifyServiceState')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__notify_service_state
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__notify_service_state
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__notify_service_state
            cls._TYPE_SUPPORT = module.type_support_msg__msg__notify_service_state
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__notify_service_state

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class NotifyServiceState(metaclass=Metaclass_NotifyServiceState):
    """Message class 'NotifyServiceState'."""

    __slots__ = [
        '_os_type',
        '_service_list',
        '_service_state_list',
    ]

    _fields_and_field_types = {
        'os_type': 'int32',
        'service_list': 'sequence<string>',
        'service_state_list': 'sequence<int32>',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.UnboundedString()),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.BasicType('int32')),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.os_type = kwargs.get('os_type', int())
        self.service_list = kwargs.get('service_list', [])
        self.service_state_list = array.array('i', kwargs.get('service_state_list', []))

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
        if self.os_type != other.os_type:
            return False
        if self.service_list != other.service_list:
            return False
        if self.service_state_list != other.service_state_list:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def os_type(self):
        """Message field 'os_type'."""
        return self._os_type

    @os_type.setter
    def os_type(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'os_type' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'os_type' field must be an integer in [-2147483648, 2147483647]"
        self._os_type = value

    @builtins.property
    def service_list(self):
        """Message field 'service_list'."""
        return self._service_list

    @service_list.setter
    def service_list(self, value):
        if __debug__:
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
                 all(isinstance(v, str) for v in value) and
                 True), \
                "The 'service_list' field must be a set or sequence and each value of type 'str'"
        self._service_list = value

    @builtins.property
    def service_state_list(self):
        """Message field 'service_state_list'."""
        return self._service_state_list

    @service_state_list.setter
    def service_state_list(self, value):
        if isinstance(value, array.array):
            assert value.typecode == 'i', \
                "The 'service_state_list' array.array() must have the type code of 'i'"
            self._service_state_list = value
            return
        if __debug__:
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
                 all(isinstance(v, int) for v in value) and
                 all(val >= -2147483648 and val < 2147483648 for val in value)), \
                "The 'service_state_list' field must be a set or sequence and each value of type 'int' and each integer in [-2147483648, 2147483647]"
        self._service_state_list = array.array('i', value)
