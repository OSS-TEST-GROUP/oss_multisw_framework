# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mmf_interfaces:msg/DetectionReport.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_DetectionReport(type):
    """Metaclass of message 'DetectionReport'."""

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
                'mmf_interfaces.msg.DetectionReport')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__detection_report
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__detection_report
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__detection_report
            cls._TYPE_SUPPORT = module.type_support_msg__msg__detection_report
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__detection_report

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class DetectionReport(metaclass=Metaclass_DetectionReport):
    """Message class 'DetectionReport'."""

    __slots__ = [
        '_client_id',
        '_dds_topic',
        '_rule',
        '_timestamp_ms',
    ]

    _fields_and_field_types = {
        'client_id': 'string',
        'dds_topic': 'string',
        'rule': 'string',
        'timestamp_ms': 'int64',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('int64'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.client_id = kwargs.get('client_id', str())
        self.dds_topic = kwargs.get('dds_topic', str())
        self.rule = kwargs.get('rule', str())
        self.timestamp_ms = kwargs.get('timestamp_ms', int())

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
        if self.client_id != other.client_id:
            return False
        if self.dds_topic != other.dds_topic:
            return False
        if self.rule != other.rule:
            return False
        if self.timestamp_ms != other.timestamp_ms:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def client_id(self):
        """Message field 'client_id'."""
        return self._client_id

    @client_id.setter
    def client_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'client_id' field must be of type 'str'"
        self._client_id = value

    @builtins.property
    def dds_topic(self):
        """Message field 'dds_topic'."""
        return self._dds_topic

    @dds_topic.setter
    def dds_topic(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'dds_topic' field must be of type 'str'"
        self._dds_topic = value

    @builtins.property
    def rule(self):
        """Message field 'rule'."""
        return self._rule

    @rule.setter
    def rule(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'rule' field must be of type 'str'"
        self._rule = value

    @builtins.property
    def timestamp_ms(self):
        """Message field 'timestamp_ms'."""
        return self._timestamp_ms

    @timestamp_ms.setter
    def timestamp_ms(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'timestamp_ms' field must be of type 'int'"
            assert value >= -9223372036854775808 and value < 9223372036854775808, \
                "The 'timestamp_ms' field must be an integer in [-9223372036854775808, 9223372036854775807]"
        self._timestamp_ms = value
