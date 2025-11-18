# Copyright 2025 Zhexuan Yang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Service call playback.
"""

import rospy
from typing import Dict
from .._reader import Reader
from .._writer import StorageOptions
from std_msgs.msg import String
import json


class ServicePlayer:
    """Plays back recorded service calls."""
    
    def __init__(self, reader: Reader):
        """
        Initialize service player.
        
        Args:
            reader: Reader instance to read service calls
        """
        self.reader = reader
        self.service_publishers: Dict[str, rospy.Publisher] = {}
    
    def setup_service_publishers(self):
        """Create publishers for service event topics."""
        # Get all service event topics from bag
        topics = self.reader.get_topics()
        
        for topic in topics:
            if topic.name.startswith('/service_events'):
                pub = rospy.Publisher(topic.name, String, queue_size=10)
                self.service_publishers[topic.name] = pub
                rospy.loginfo(f"Publishing service events: {topic.name}")
    
    def play(self):
        """Play back service calls"""
        for msg_data in self.reader.read_messages():
            if msg_data.topic.startswith('/service_events'):
                # Publish service event
                if msg_data.topic in self.service_publishers:
                    # Deserialize message
                    msg = String()
                    msg.data = msg_data.data.decode('utf-8')
                    self.service_publishers[msg_data.topic].publish(msg)
                
                rospy.sleep(0.01)  # Small delay between calls

