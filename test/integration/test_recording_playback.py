"""
Integration tests for recording and playback.
"""

import unittest
import tempfile
import os
import rospy
from std_msgs.msg import String, Int32
from rosbag1_py import Writer, Reader, StorageOptions, TopicMetadata


class TestRecordingPlayback(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        if not rospy.get_node_uri():
            rospy.init_node('test_recording_playback', anonymous=True)
        
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_record_and_read_single_topic(self):
        """Test recording and reading a single topic"""
        # Record
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/chatter',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write some messages
        for i in range(5):
            msg = String()
            msg.data = f"Message {i}"
            writer.write_message('/chatter', msg)
        
        writer.close()
        
        # Read back
        reader = Reader()
        reader.open(storage_opts)
        
        topics = reader.get_topics()
        self.assertGreater(len(topics), 0)
        
        message_count = 0
        for msg_data in reader.read_messages():
            message_count += 1
            self.assertEqual(msg_data.topic, '/chatter')
        
        self.assertGreater(message_count, 0)
        reader.close()
    
    def test_record_and_read_multiple_topics(self):
        """Test recording and reading multiple topics"""
        # Record
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
        
        # Write messages to both topics
        for i in range(3):
            msg1 = String()
            msg1.data = f"String {i}"
            writer.write_message('/topic1', msg1)
            
            msg2 = Int32()
            msg2.data = i
            writer.write_message('/topic2', msg2)
        
        writer.close()
        
        # Read back
        reader = Reader()
        reader.open(storage_opts)
        
        topics = reader.get_topics()
        self.assertGreaterEqual(len(topics), 2)
        
        topic1_count = 0
        topic2_count = 0
        
        for msg_data in reader.read_messages():
            if msg_data.topic == '/topic1':
                topic1_count += 1
            elif msg_data.topic == '/topic2':
                topic2_count += 1
        
        self.assertGreater(topic1_count, 0)
        self.assertGreater(topic2_count, 0)
        reader.close()
    
    def test_topic_filtering(self):
        """Test reading with topic filters"""
        # Record multiple topics
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        for i in range(3):
            topic_meta = TopicMetadata(
                id=i,
                name=f'/topic{i}',
                type='std_msgs/String',
                serialization_format='cdr'
            )
            writer.create_topic(topic_meta)
            
            msg = String()
            msg.data = f"Message for topic {i}"
            writer.write_message(f'/topic{i}', msg)
        
        writer.close()
        
        # Read with filter
        reader = Reader()
        reader.open(storage_opts)
        
        filtered_messages = list(reader.read_messages(topic_filters=['/topic0', '/topic2']))
        
        for msg_data in filtered_messages:
            self.assertIn(msg_data.topic, ['/topic0', '/topic2'])
        
        reader.close()


if __name__ == '__main__':
    unittest.main()

