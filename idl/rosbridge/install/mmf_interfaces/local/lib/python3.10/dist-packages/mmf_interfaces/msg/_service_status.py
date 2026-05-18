# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mmf_interfaces:msg/ServiceStatus.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_ServiceStatus(type):
    """Metaclass of message 'ServiceStatus'."""

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
                'mmf_interfaces.msg.ServiceStatus')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__service_status
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__service_status
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__service_status
            cls._TYPE_SUPPORT = module.type_support_msg__msg__service_status
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__service_status

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class ServiceStatus(metaclass=Metaclass_ServiceStatus):
    """Message class 'ServiceStatus'."""

    __slots__ = [
        '_name',
        '_service_type',
        '_service_state',
        '_is_enabled',
        '_recovered_count',
    ]

    _fields_and_field_types = {
        'name': 'string',
        'service_type': 'int32',
        'service_state': 'int32',
        'is_enabled': 'boolean',
        'recovered_count': 'int32',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.name = kwargs.get('name', str())
        self.service_type = kwargs.get('service_type', int())
        self.service_state = kwargs.get('service_state', int())
        self.is_enabled = kwargs.get('is_enabled', bool())
        self.recovered_count = kwargs.get('recovered_count', int())

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
        if self.name != other.name:
            return False
        if self.service_type != other.service_type:
            return False
        if self.service_state != other.service_state:
            return False
        if self.is_enabled != other.is_enabled:
            return False
        if self.recovered_count != other.recovered_count:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def name(self):
        """Message field 'name'."""
        return self._name

    @name.setter
    def name(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'name' field must be of type 'str'"
        self._name = value

    @builtins.property
    def service_type(self):
        """Message field 'service_type'."""
        return self._service_type

    @service_type.setter
    def service_type(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'service_type' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'service_type' field must be an integer in [-2147483648, 2147483647]"
        self._service_type = value

    @builtins.property
    def service_state(self):
        """Message field 'service_state'."""
        return self._service_state

    @service_state.setter
    def service_state(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'service_state' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'service_state' field must be an integer in [-2147483648, 2147483647]"
        self._service_state = value

    @builtins.property
    def is_enabled(self):
        """Message field 'is_enabled'."""
        return self._is_enabled

    @is_enabled.setter
    def is_enabled(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'is_enabled' field must be of type 'bool'"
        self._is_enabled = value

    @builtins.property
    def recovered_count(self):
        """Message field 'recovered_count'."""
        return self._recovered_count

    @recovered_count.setter
    def recovered_count(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'recovered_count' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'recovered_count' field must be an integer in [-2147483648, 2147483647]"
        self._recovered_count = value
