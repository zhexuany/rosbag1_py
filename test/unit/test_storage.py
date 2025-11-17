"""
Python unit tests for storage backends.
"""

import unittest
import tempfile
import os
from rosbag1_py import Writer, StorageOptions, TopicMetadata


class TestStorage(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_mcap_storage(self):
        """Test MCAP storage backend"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # Check that MCAP file was created
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_rosbag_storage(self):
        """Test ROSBAG storage backend"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='rosbag'
        )
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # Check that bag file was created
        # Note: Actual file extension depends on implementation
        self.assertTrue(os.path.exists(f"{self.bag_path}.bag") or 
                       os.path.exists(f"{self.bag_path}"))


if __name__ == '__main__':
    unittest.main()

