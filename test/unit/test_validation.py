"""
Unit tests for bag validation utilities.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, StorageOptions, TopicMetadata
from rosbag1_py.utils.validation import validate_bag


class TestValidation(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        if not rospy.get_node_uri():
            rospy.init_node('test_validation', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_validate_valid_bag(self):
        """Test validation of a valid bag"""
        # Create a valid bag
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write some messages
        for i in range(3):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/test_topic', msg)
        
        writer.close()
        
        # Validate bag
        result = validate_bag(self.bag_path, storage_id='mcap')
        
        self.assertIsNotNone(result)
        self.assertIn('valid', result)
        self.assertIn('message_count', result)
        self.assertIn('topic_count', result)
        self.assertEqual(result['message_count'], 3)
        self.assertEqual(result['topic_count'], 1)
    
    def test_validate_empty_bag(self):
        """Test validation of an empty bag"""
        # Create an empty bag
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        writer.close()
        
        # Validate bag
        result = validate_bag(self.bag_path, storage_id='mcap')
        
        self.assertIsNotNone(result)
        self.assertIn('valid', result)
        self.assertIn('warnings', result)
        # Empty bag should have warnings
        self.assertGreater(len(result['warnings']), 0)
    
    def test_validate_nonexistent_bag(self):
        """Test validation of a non-existent bag"""
        result = validate_bag('/nonexistent/bag.mcap', storage_id='mcap')
        
        self.assertIsNotNone(result)
        self.assertIn('valid', result)
        self.assertFalse(result['valid'])
        self.assertIn('errors', result)
        self.assertGreater(len(result['errors']), 0)


if __name__ == '__main__':
    unittest.main()

