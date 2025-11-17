"""
Python unit tests for Writer.
"""

import unittest
import tempfile
import os
from rosbag1_py import Writer, StorageOptions, TopicMetadata, CompressionOptions


class TestWriter(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_basic_creation(self):
        """Test basic writer creation"""
        writer = Writer()
        self.assertFalse(writer.is_open())
    
    def test_open_close(self):
        """Test opening and closing bag"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        writer.open(storage_opts)
        self.assertTrue(writer.is_open())
        writer.close()
        self.assertFalse(writer.is_open())
    
    def test_create_topic(self):
        """Test topic creation"""
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
        self.assertTrue(writer.is_open())
        writer.close()


if __name__ == '__main__':
    unittest.main()

