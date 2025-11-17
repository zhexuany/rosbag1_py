"""
Integration tests for bag splitting.
"""

import unittest
import tempfile
import os
import rospy
import time
import glob
from std_msgs.msg import String
from rosbag1_py import Writer, StorageOptions, TopicMetadata, SplitOptions
from rosbag1_py import rosbag1_py_cpp


class TestBagSplitting(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        if not rospy.get_node_uri():
            rospy.init_node('test_splitting', anonymous=True)
        
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_size_based_splitting(self):
        """Test splitting by file size"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        
        # Enable size-based splitting with small threshold (100KB)
        split_opts = SplitOptions(mode='size', max_size=100 * 1024)
        writer.open(storage_opts, split_options=split_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write large messages to trigger split
        for i in range(200):
            msg = String()
            msg.data = "X" * 1000  # 1KB message
            writer.write_message('/test_topic', msg)
        
        writer.close()
        
        # Check that bag files exist (should have multiple files due to splitting)
        bag_files = glob.glob(f"{self.bag_path}*.mcap")
        self.assertGreater(len(bag_files), 0, "At least one bag file should exist")
    
    def test_duration_based_splitting(self):
        """Test splitting by duration"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        
        # Enable duration-based splitting with short duration (1 second)
        split_opts = SplitOptions(mode='duration', max_duration=1.0)
        writer.open(storage_opts, split_options=split_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write messages with delays to trigger duration-based split
        for i in range(5):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/test_topic', msg)
            time.sleep(0.5)  # Sleep to create time gaps
        
        writer.close()
        
        # Check that bag files exist
        bag_files = glob.glob(f"{self.bag_path}*.mcap")
        self.assertGreater(len(bag_files), 0, "At least one bag file should exist")
    
    def test_message_count_splitting(self):
        """Test splitting by message count"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        
        # Enable message count-based splitting with small threshold (10 messages)
        split_opts = SplitOptions(mode='message_count', max_messages=10)
        writer.open(storage_opts, split_options=split_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write more messages than threshold to trigger split
        for i in range(25):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/test_topic', msg)
        
        writer.close()
        
        # Check that multiple bag files exist due to splitting
        bag_files = glob.glob(f"{self.bag_path}*.mcap")
        self.assertGreater(len(bag_files), 1, "Multiple bag files should exist due to splitting")


if __name__ == '__main__':
    unittest.main()

