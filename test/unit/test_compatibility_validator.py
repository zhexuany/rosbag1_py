"""
Unit tests for bag compatibility validator.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, StorageOptions, TopicMetadata
from rosbag1_py.compatibility.validator import validate_bag_compatibility


class TestCompatibilityValidator(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        if not rospy.get_node_uri():
            rospy.init_node('test_compatibility_validator', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_validate_ros1_bag_for_ros2(self):
        """Test validating ROS1 bag for ROS2 compatibility"""
        # Create a ROS1 bag
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',  # ROS1 type
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # Validate for ROS2
        result = validate_bag_compatibility(self.bag_path, target_ros=2, storage_id='mcap')
        
        self.assertIsNotNone(result)
        self.assertIn('compatible', result)
        self.assertIn('compatible_topics', result)
        self.assertIn('incompatible_topics', result)
        self.assertIn('total_topics', result)
        self.assertEqual(result['total_topics'], 1)
    
    def test_validate_empty_bag(self):
        """Test validating an empty bag"""
        # Create an empty bag
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        writer.close()
        
        # Validate
        result = validate_bag_compatibility(self.bag_path, target_ros=2, storage_id='mcap')
        
        self.assertIsNotNone(result)
        self.assertIn('compatible', result)
        self.assertEqual(result['total_topics'], 0)


if __name__ == '__main__':
    unittest.main()

