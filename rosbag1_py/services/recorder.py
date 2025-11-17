"""
Service call recording.
"""

import rospy
from typing import Dict, Any, Optional, Callable
from .._writer import Writer, TopicMetadata, StorageOptions
import json


class ServiceRecorder:
    """Records ROS service calls to a bag."""
    
    def __init__(self, writer: Writer):
        """
        Initialize service recorder.
        
        Args:
            writer: Writer instance to record service calls
        """
        self.writer = writer
        self.services: Dict[str, Dict[str, Any]] = {}
        self.service_id_counter = 0
    
    def register_service(self, service_name: str, service_type) -> None:
        """
        Register a service for recording.
        
        Args:
            service_name: Name of the service
            service_type: Service type class
        """
        # Get type information
        req_class = service_type._request_class
        resp_class = service_type._response_class
        
        # Create metadata
        metadata = {
            "id": self.service_id_counter,
            "name": service_name,
            "service_type": service_type._type,
            "request_type": req_class._type,
            "response_type": resp_class._type,
            "md5sum": service_type._md5sum
        }
        
        self.service_id_counter += 1
        
        # Store service metadata in bag as special topic
        # Format: /service_events/<service_name>
        topic_name = f"/service_events{service_name}"
        
        topic_meta = TopicMetadata(
            id=metadata["id"],
            name=topic_name,
            type='rosbag1_py/ServiceCall',  # Custom message type
            serialization_format='cdr'
        )
        self.writer.create_topic(topic_meta)
        
        self.services[service_name] = metadata
        rospy.loginfo(f"Registered service for recording: {service_name}")
    
    def create_recording_proxy(self, service_name: str, service_type) -> Callable:
        """
        Create a wrapper function that records service calls.
        
        Args:
            service_name: Name of the service
            service_type: Service type class
        
        Returns:
            Callable that acts like a service proxy but records calls
        """
        # Create actual proxy
        proxy = rospy.ServiceProxy(service_name, service_type)
        
        def recording_wrapper(*args, **kwargs):
            """Wrapper that records the call"""
            timestamp = rospy.Time.now()
            
            try:
                # Make actual service call
                response = proxy(*args, **kwargs)
                success = True
            except rospy.ServiceException as e:
                rospy.logerr(f"Service call failed: {e}")
                response = None
                success = False
            
            # Record the call
            self._record_service_call(
                service_name,
                args[0] if args else None,  # Request
                response,
                timestamp,
                success
            )
            
            if not success:
                raise rospy.ServiceException(f"Service call failed")
            
            return response
        
        return recording_wrapper
    
    def _record_service_call(
        self,
        service_name: str,
        request,
        response,
        timestamp: rospy.Time,
        success: bool
    ):
        """Record a service call to the bag"""
        topic_name = f"/service_events{service_name}"
        
        # Create service call message
        # Note: In a full implementation, a custom rosbag1_py/ServiceCall message type
        # would be defined with fields: timestamp, service_name, success, request, response
        # For now, we use std_msgs/String with JSON encoding as a practical solution
        from std_msgs.msg import String
        
        call_data = {
            'timestamp': timestamp.to_sec(),
            'service_name': service_name,
            'success': success,
            'request': str(request) if request else None,
            'response': str(response) if response else None,
        }
        
        msg = String()
        msg.data = json.dumps(call_data)
        
        # Write to bag
        self.writer.write_message(topic_name, msg, timestamp)

