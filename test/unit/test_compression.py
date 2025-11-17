"""
Python unit tests for compression.
"""

import unittest
import rospy
from rosbag1_py import Writer, StorageOptions, TopicMetadata, CompressionOptions
from std_msgs.msg import String
import tempfile
import os


class TestCompression(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        if not rospy.get_node_uri():
            rospy.init_node('test_compression', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_no_compression(self):
        """Test recording without compression"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        compression_opts = CompressionOptions(compression_mode='none')
        
        writer.open(storage_opts, compression_options=compression_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write at least one message so file is created
        msg = String()
        msg.data = "test"
        writer.write_message('/test_topic', msg)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_lz4_compression(self):
        """Test LZ4 compression"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        compression_opts = CompressionOptions(
            compression_mode='lz4',
            compression_level=4
        )
        
        writer.open(storage_opts, compression_options=compression_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write at least one message so file is created
        msg = String()
        msg.data = "test"
        writer.write_message('/test_topic', msg)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_zstd_compression(self):
        """Test ZSTD compression"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        compression_opts = CompressionOptions(
            compression_mode='zstd',
            compression_level=3
        )
        
        writer.open(storage_opts, compression_options=compression_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write at least one message so file is created
        msg = String()
        msg.data = "test"
        writer.write_message('/test_topic', msg)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_bz2_compression(self):
        """Test BZ2 compression"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        compression_opts = CompressionOptions(
            compression_mode='bz2',
            compression_level=9
        )
        
        writer.open(storage_opts, compression_options=compression_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write at least one message so file is created
        msg = String()
        msg.data = "test"
        writer.write_message('/test_topic', msg)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))


if __name__ == '__main__':
    unittest.main()

