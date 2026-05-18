# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mmf_interfaces:msg/RecoveryPolicy.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_RecoveryPolicy(type):
    """Metaclass of message 'RecoveryPolicy'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
        'R_DEFAULT': 0,
        'ALWAYS': 1,
        'ON_FAILURE': 2,
        'ON_SUCCESS': 3,
        'ON_NOT_RESPONDING': 4,
        'MANUAL': 5,
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
                'mmf_interfaces.msg.RecoveryPolicy')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__recovery_policy
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__recovery_policy
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__recovery_policy
            cls._TYPE_SUPPORT = module.type_support_msg__msg__recovery_policy
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__recovery_policy

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
            'R_DEFAULT': cls.__constants['R_DEFAULT'],
            'ALWAYS': cls.__constants['ALWAYS'],
            'ON_FAILURE': cls.__constants['ON_FAILURE'],
            'ON_SUCCESS': cls.__constants['ON_SUCCESS'],
            'ON_NOT_RESPONDING': cls.__constants['ON_NOT_RESPONDING'],
            'MANUAL': cls.__constants['MANUAL'],
        }

    @property
    def R_DEFAULT(self):
        """Message constant 'R_DEFAULT'."""
        return Metaclass_RecoveryPolicy.__constants['R_DEFAULT']

    @property
    def ALWAYS(self):
        """Message constant 'ALWAYS'."""
        return Metaclass_RecoveryPolicy.__constants['ALWAYS']

    @property
    def ON_FAILURE(self):
        """Message constant 'ON_FAILURE'."""
        return Metaclass_RecoveryPolicy.__constants['ON_FAILURE']

    @property
    def ON_SUCCESS(self):
        """Message constant 'ON_SUCCESS'."""
        return Metaclass_RecoveryPolicy.__constants['ON_SUCCESS']

    @property
    def ON_NOT_RESPONDING(self):
        """Message constant 'ON_NOT_RESPONDING'."""
        return Metaclass_RecoveryPolicy.__constants['ON_NOT_RESPONDING']

    @property
    def MANUAL(self):
        """Message constant 'MANUAL'."""
        return Metaclass_RecoveryPolicy.__constants['MANUAL']


class RecoveryPolicy(metaclass=Metaclass_RecoveryPolicy):
    """
    Message class 'RecoveryPolicy'.

    Constants:
      R_DEFAULT
      ALWAYS
      ON_FAILURE
      ON_SUCCESS
      ON_NOT_RESPONDING
      MANUAL
    """

    __slots__ = [
        '_value',
    ]

    _fields_and_field_types = {
        'value': 'int32',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.value = kwargs.get('value', int())

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
        if self.value != other.value:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def value(self):
        """Message field 'value'."""
        return self._value

    @value.setter
    def value(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'value' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'value' field must be an integer in [-2147483648, 2147483647]"
        self._value = value
