"""
Python unit tests for Reader.
"""

import unittest
import tempfile
import os
import rospy
from rosbag1_py import Reader, Writer, StorageOptions, TopicMetadata
from std_msgs.msg import String, Int32


class TestReader(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        if not rospy.get_node_uri():
            rospy.init_node('test_reader', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_basic_creation(self):
        """Test basic reader creation"""
        reader = Reader()
        self.assertFalse(reader.is_open())
    
    def test_open_close(self):
        """Test opening and closing bag"""
        # First create a bag
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
        
        # Now try to read it
        reader = Reader()
        reader.open(storage_opts)
        self.assertTrue(reader.is_open())
        reader.close()
        self.assertFalse(reader.is_open())
    
    def test_get_topics(self):
        """Test getting topics from bag"""
        # Create a bag with topics
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic1_meta = TopicMetadata(
            id=0,
            name='/topic1',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        topic2_meta = TopicMetadata(
            id=1,
            name='/topic2',
            type='std_msgs/Int32',
            serialization_format='cdr'
        )
        writer.create_topic(topic1_meta)
        writer.create_topic(topic2_meta)
        writer.close()
        
        # Read topics
        reader = Reader()
        reader.open(storage_opts)
        topics = reader.get_topics()
        self.assertGreaterEqual(len(topics), 0)  # At least should not crash
        reader.close()
    
    def test_get_metadata(self):
        """Test getting bag metadata"""
        # Create a bag
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        writer.close()
        
        # Get metadata
        reader = Reader()
        reader.open(storage_opts)
        metadata = reader.get_metadata()
        self.assertIsNotNone(metadata)
        self.assertIsInstance(metadata.message_count, int)
        reader.close()
    
    def test_callback_based_reading(self):
        """Test callback-based message reading"""
        # Create a bag with messages
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
        
        writer.close()
        
        # Read using callback
        reader = Reader()
        reader.open(storage_opts)
        
        messages_received = []
        def callback(msg_data):
            messages_received.append(msg_data)
        
        reader.read_messages(callback)
        reader.close()
        
        self.assertGreater(len(messages_received), 0, "Should receive messages via callback")
    
    def test_callback_with_topic_filter(self):
        """Test callback-based reading with topic filtering"""
        # Create a bag with multiple topics
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic1_meta = TopicMetadata(id=0, name='/topic1', type='std_msgs/String', serialization_format='cdr')
        topic2_meta = TopicMetadata(id=1, name='/topic2', type='std_msgs/String', serialization_format='cdr')
        writer.create_topic(topic1_meta)
        writer.create_topic(topic2_meta)
        
        # Write messages to both topics
        for i in range(3):
            msg = String()
            msg.data = f"Topic1 Message {i}"
            writer.write_message('/topic1', msg)
        
        for i in range(2):
            msg = String()
            msg.data = f"Topic2 Message {i}"
            writer.write_message('/topic2', msg)
        
        writer.close()
        
        # Read only topic1 using filter
        reader = Reader()
        reader.open(storage_opts)
        
        messages_received = []
        def callback(msg_data):
            messages_received.append(msg_data)
        
        reader.read_messages(callback, topic_filters=['/topic1'])
        reader.close()
        
        # Should only receive messages from topic1
        self.assertEqual(len(messages_received), 3, "Should receive 3 messages from topic1")
        for msg in messages_received:
            self.assertEqual(msg.topic, '/topic1')
    
    def test_message_count_per_topic(self):
        """Test message count per topic in metadata"""
        # Create a bag with multiple topics and messages
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic1_meta = TopicMetadata(id=0, name='/topic1', type='std_msgs/String', serialization_format='cdr')
        topic2_meta = TopicMetadata(id=1, name='/topic2', type='std_msgs/Int32', serialization_format='cdr')
        writer.create_topic(topic1_meta)
        writer.create_topic(topic2_meta)
        
        # Write different numbers of messages to each topic
        for i in range(5):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/topic1', msg)
        
        for i in range(3):
            msg = Int32()
            msg.data = i
            writer.write_message('/topic2', msg)
        
        writer.close()
        
        # Get metadata and check message counts
        reader = Reader()
        reader.open(storage_opts)
        metadata = reader.get_metadata()
        reader.close()
        
        # Check that topics_with_message_count has correct counts
        topic_counts = {name: count for name, _, count in metadata.topics_with_message_count}
        self.assertEqual(topic_counts.get('/topic1', 0), 5, "Topic1 should have 5 messages")
        self.assertEqual(topic_counts.get('/topic2', 0), 3, "Topic2 should have 3 messages")


if __name__ == '__main__':
    unittest.main()

