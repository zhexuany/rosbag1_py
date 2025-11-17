"""
Integration tests for service player.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, Reader, StorageOptions, TopicMetadata
from rosbag1_py.services.player import ServicePlayer


class TestServicePlayer(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        if not rospy.get_node_uri():
            rospy.init_node('test_service_player', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'service_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_service_player_creation(self):
        """Test ServicePlayer initialization"""
        # Create a bag with service events
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/service_events/test_service',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # Create reader and player
        reader = Reader()
        reader.open(storage_opts)
        player = ServicePlayer(reader)
        
        self.assertIsNotNone(player)
        self.assertIsNotNone(player.reader)
        reader.close()
    
    def test_setup_service_publishers(self):
        """Test setting up service publishers"""
        # Create a bag with service events
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/service_events/test_service',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # Create reader and player
        reader = Reader()
        reader.open(storage_opts)
        player = ServicePlayer(reader)
        
        # Setup publishers
        player.setup_service_publishers()
        
        # Check that publishers were created
        self.assertGreater(len(player.service_publishers), 0)
        reader.close()


if __name__ == '__main__':
    unittest.main()

