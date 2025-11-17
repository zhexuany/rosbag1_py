"""
Type name mapping between ROS1 and ROS2.
"""

from typing import Dict
from .. import rosbag1_py_cpp


class TypeConverter:
    """Convert type names between ROS1 and ROS2."""
    
    @staticmethod
    def ros1_to_ros2(type_name: str) -> str:
        """Convert ROS1 type name to ROS2 type name."""
        return rosbag1_py_cpp.TypeConverter.ros1_to_ros2(type_name)
    
    @staticmethod
    def ros2_to_ros1(type_name: str) -> str:
        """Convert ROS2 type name to ROS1 type name."""
        return rosbag1_py_cpp.TypeConverter.ros2_to_ros1(type_name)
    
    @staticmethod
    def is_ros1_type(type_name: str) -> bool:
        """Check if type is ROS1 format."""
        return rosbag1_py_cpp.TypeConverter.is_ros1_type(type_name)
    
    @staticmethod
    def is_ros2_type(type_name: str) -> bool:
        """Check if type is ROS2 format."""
        return rosbag1_py_cpp.TypeConverter.is_ros2_type(type_name)


# Convenience functions
def ros1_to_ros2(type_name: str) -> str:
    """Convert ROS1 type name to ROS2 type name."""
    return TypeConverter.ros1_to_ros2(type_name)


def ros2_to_ros1(type_name: str) -> str:
    """Convert ROS2 type name to ROS1 type name."""
    return TypeConverter.ros2_to_ros1(type_name)

