# Copyright 2025 Zhexuan Yang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

