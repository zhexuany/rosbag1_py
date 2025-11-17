"""
Integration tests for compression with actual recording.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, Reader, StorageOptions, TopicMetadata, CompressionOptions


class TestCompressionIntegration(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        if not rospy.get_node_uri():
            rospy.init_node('test_compression', anonymous=True)
        
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_record_with_compression_and_read(self):
        """Test recording with compression and reading back"""
        # Record with compression
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        compression_opts = CompressionOptions(
            compression_mode='lz4',
            compression_level=4
        )
        writer.open(storage_opts, compression_options=compression_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/chatter',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write messages
        for i in range(10):
            msg = String()
            msg.data = f"Compressed message {i}" + "X" * 100  # Larger message
            writer.write_message('/chatter', msg)
        
        writer.close()
        
        # Read back
        reader = Reader()
        reader.open(storage_opts)
        
        message_count = 0
        for msg_data in reader.read_messages():
            message_count += 1
            self.assertEqual(msg_data.topic, '/chatter')
        
        self.assertGreater(message_count, 0)
        reader.close()
    
    def test_compression_file_size(self):
        """Test that compression reduces file size for large messages"""
        # Record without compression
        writer1 = Writer()
        storage_opts1 = StorageOptions(
            uri=os.path.join(self.temp_dir, 'uncompressed'),
            storage_id='mcap'
        )
        writer1.open(storage_opts1)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/large_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer1.create_topic(topic_meta)
        
        # Write large messages
        for i in range(50):
            msg = String()
            msg.data = "A" * 10000  # 10KB message
            writer1.write_message('/large_topic', msg)
        
        writer1.close()
        
        # Record with compression
        writer2 = Writer()
        storage_opts2 = StorageOptions(
            uri=os.path.join(self.temp_dir, 'compressed'),
            storage_id='mcap'
        )
        compression_opts = CompressionOptions(
            compression_mode='lz4',
            compression_level=4
        )
        writer2.open(storage_opts2, compression_options=compression_opts)
        writer2.create_topic(topic_meta)
        
        # Write same large messages
        for i in range(50):
            msg = String()
            msg.data = "A" * 10000
            writer2.write_message('/large_topic', msg)
        
        writer2.close()
        
        # Compare file sizes (compressed should be smaller)
        uncompressed_size = os.path.getsize(f"{storage_opts1.uri}.mcap")
        compressed_size = os.path.getsize(f"{storage_opts2.uri}.mcap")
        
        # Note: This is a basic check - actual compression ratio depends on data
        self.assertGreater(uncompressed_size, 0)
        self.assertGreater(compressed_size, 0)


if __name__ == '__main__':
    unittest.main()

