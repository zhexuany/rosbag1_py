"""
Compatibility tests for ROS1/ROS2 type conversion.
"""

import unittest
from rosbag1_py.compatibility.type_mapping import TypeConverter


class TestTypeConversion(unittest.TestCase):
    
    def test_ros1_to_ros2_conversion(self):
        """Test converting ROS1 type names to ROS2"""
        # Standard types
        self.assertEqual(
            TypeConverter.ros1_to_ros2('std_msgs/String'),
            'std_msgs/msg/String'
        )
        self.assertEqual(
            TypeConverter.ros1_to_ros2('sensor_msgs/Image'),
            'sensor_msgs/msg/Image'
        )
        self.assertEqual(
            TypeConverter.ros1_to_ros2('geometry_msgs/Pose'),
            'geometry_msgs/msg/Pose'
        )
    
    def test_ros2_to_ros1_conversion(self):
        """Test converting ROS2 type names to ROS1"""
        # Standard types
        self.assertEqual(
            TypeConverter.ros2_to_ros1('std_msgs/msg/String'),
            'std_msgs/String'
        )
        self.assertEqual(
            TypeConverter.ros2_to_ros1('sensor_msgs/msg/Image'),
            'sensor_msgs/Image'
        )
    
    def test_round_trip_conversion(self):
        """Test round-trip conversion"""
        ros1_types = [
            'std_msgs/String',
            'sensor_msgs/Image',
            'geometry_msgs/Pose',
            'nav_msgs/Odometry'
        ]
        
        for ros1_type in ros1_types:
            ros2_type = TypeConverter.ros1_to_ros2(ros1_type)
            converted_back = TypeConverter.ros2_to_ros1(ros2_type)
            self.assertEqual(ros1_type, converted_back)
    
    def test_type_detection(self):
        """Test detecting ROS1 vs ROS2 types"""
        # ROS1 types
        self.assertTrue(TypeConverter.is_ros1_type('std_msgs/String'))
        self.assertTrue(TypeConverter.is_ros1_type('sensor_msgs/Image'))
        self.assertFalse(TypeConverter.is_ros2_type('std_msgs/String'))
        
        # ROS2 types
        self.assertTrue(TypeConverter.is_ros2_type('std_msgs/msg/String'))
        self.assertTrue(TypeConverter.is_ros2_type('sensor_msgs/msg/Image'))
        self.assertFalse(TypeConverter.is_ros1_type('std_msgs/msg/String'))


if __name__ == '__main__':
    unittest.main()

