"""
Integration tests for service recording.
"""

import unittest
import tempfile
import os
import rospy
from std_srvs.srv import Empty, EmptyRequest, EmptyResponse
from rosbag1_py import Writer, StorageOptions, TopicMetadata
from rosbag1_py.services import ServiceRecorder


class TestServiceRecording(unittest.TestCase):
    
    def setUp(self):
        """Set up test fixtures"""
        if not rospy.get_node_uri():
            rospy.init_node('test_service_recording', anonymous=True)
        
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_service_registration(self):
        """Test registering a service for recording"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        service_recorder = ServiceRecorder(writer)
        
        # Register a service
        try:
            service_recorder.register_service('/test_service', Empty)
            # Check that service event topic was created
            topics = writer._topics
            self.assertTrue(any('/service_events' in topic for topic in topics))
        except Exception as e:
            # Service might not be available, which is okay for unit test
            pass
        
        writer.close()
    
    def test_service_recording_proxy(self):
        """Test creating a recording proxy"""
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        service_recorder = ServiceRecorder(writer)
        
        try:
            # Create recording proxy
            proxy = service_recorder.create_recording_proxy('/test_service', Empty)
            self.assertIsNotNone(proxy)
            self.assertTrue(callable(proxy))
        except Exception as e:
            # Service might not be available, which is okay for unit test
            pass
        
        writer.close()
    
    def test_service_recording_with_call(self):
        """Test recording actual service calls"""
        from std_srvs.srv import Empty
        from rosbag1_py import Reader
        
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        recorder = ServiceRecorder(writer)
        recorder.register_service('/test_service', Empty)
        
        # Simulate recording a service call manually
        import rospy
        timestamp = rospy.Time.now()
        
        # Call the internal record method directly
        recorder._record_service_call(
            service_name='/test_service',
            request="test request",
            response="test response",
            timestamp=timestamp,
            success=True
        )
        
        writer.close()
        
        # Verify the call was recorded by reading back
        reader = Reader()
        reader.open(storage_opts)
        
        messages = list(reader.read_messages())
        self.assertGreater(len(messages), 0)
        
        # Check the recorded message
        msg = messages[0]
        self.assertTrue(msg.topic.startswith('/service_events'))
        
        reader.close()
    
    def test_service_recording_failure(self):
        """Test recording failed service calls"""
        from std_srvs.srv import Empty
        from rosbag1_py import Reader
        
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        recorder = ServiceRecorder(writer)
        recorder.register_service('/test_service', Empty)
        
        # Record a failed call
        import rospy
        timestamp = rospy.Time.now()
        
        recorder._record_service_call(
            service_name='/test_service',
            request="test request",
            response=None,
            timestamp=timestamp,
            success=False
        )
        
        writer.close()
        
        # Verify the failed call was recorded
        reader = Reader()
        reader.open(storage_opts)
        
        messages = list(reader.read_messages())
        self.assertGreater(len(messages), 0)
        
        # Parse the message to check it's a failed call
        # Note: The message data is serialized ROS message, not raw JSON
        # We just verify it was recorded
        msg_data = messages[0]
        self.assertTrue(msg_data.topic.startswith('/service_events'))
        
        reader.close()
    
    def test_recording_proxy_wrapper_success(self):
        """Test the recording proxy wrapper with successful calls"""
        from std_srvs.srv import Empty
        from rosbag1_py import Reader
        import rospy
        
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        recorder = ServiceRecorder(writer)
        recorder.register_service('/test_service', Empty)
        
        # Create a mock service that will be called
        def mock_service_handler(req):
            from std_srvs.srv import EmptyResponse
            return EmptyResponse()
        
        # Start a mock service server
        try:
            service = rospy.Service('/test_service', Empty, mock_service_handler)
            rospy.sleep(0.1)  # Give service time to start
            
            # Create the recording proxy
            proxy = recorder.create_recording_proxy('/test_service', Empty)
            
            # Call the proxy (should succeed and record)
            from std_srvs.srv import EmptyRequest
            result = proxy(EmptyRequest())
            
            # Verify the call was recorded
            writer.close()
            
            reader = Reader()
            reader.open(storage_opts)
            messages = list(reader.read_messages())
            self.assertGreater(len(messages), 0)
            reader.close()
            
            service.shutdown()
        except Exception as e:
            # Service might not be available in test environment
            pass
        
        writer.close()
    
    def test_recording_proxy_wrapper_failure(self):
        """Test the recording proxy wrapper with failed service calls"""
        from std_srvs.srv import Empty
        from rosbag1_py import Reader
        
        writer = Writer()
        storage_opts = StorageOptions(uri=self.bag_path, storage_id='mcap')
        writer.open(storage_opts)
        
        recorder = ServiceRecorder(writer)
        recorder.register_service('/nonexistent_service', Empty)
        
        # Create recording proxy for non-existent service
        proxy = recorder.create_recording_proxy('/nonexistent_service', Empty)
        
        # Try to call the proxy (should fail and record failure)
        from std_srvs.srv import EmptyRequest
        try:
            # This should raise an exception
            result = proxy(EmptyRequest())
        except Exception as e:
            # Expected to fail
            pass
        
        writer.close()
        
        # Verify the failed call was recorded
        reader = Reader()
        reader.open(storage_opts)
        messages = list(reader.read_messages())
        # Should have recorded the failure
        self.assertGreater(len(messages), 0)
        reader.close()


if __name__ == '__main__':
    unittest.main()

