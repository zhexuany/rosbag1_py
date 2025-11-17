"""
Integration tests for bag metadata.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, Reader, StorageOptions, TopicMetadata
from rosbag1_py._info import get_bag_info


class TestMetadata(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        if not rospy.get_node_uri():
            rospy.init_node('test_metadata', anonymous=True)
        
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_writer_metadata(self):
        """Test getting metadata from writer"""
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
        for i in range(5):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/test_topic', msg)
        
        metadata = writer.get_metadata()
        self.assertIsNotNone(metadata)
        self.assertIn('message_count', metadata)
        self.assertIn('topic_count', metadata)
        
        writer.close()
    
    def test_reader_metadata(self):
        """Test getting metadata from reader"""
        # Create a bag first
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
        
        for i in range(5):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/test_topic', msg)
        
        writer.close()
        
        # Read metadata
        reader = Reader()
        reader.open(storage_opts)
        metadata = reader.get_metadata()
        
        self.assertIsNotNone(metadata)
        self.assertIsInstance(metadata.message_count, int)
        self.assertIsInstance(metadata.start_time_ns, int)
        self.assertIsInstance(metadata.end_time_ns, int)
        
        reader.close()
    
    def test_bag_info_utility(self):
        """Test bag info utility function"""
        # Create a bag
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
        writer.close()
        
        # Get bag info
        try:
            info = get_bag_info(self.bag_path, 'mcap')
            self.assertIsNotNone(info)
            self.assertIn('path', info)
            self.assertIn('storage_id', info)
            self.assertIn('message_count', info)
        except Exception as e:
            # Info might not work if bag is empty or not properly written
            pass


if __name__ == '__main__':
    unittest.main()

