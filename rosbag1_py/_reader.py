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
Python wrapper for C++ Reader class.
"""

import rospy
from typing import List, Optional, Callable, Tuple
from dataclasses import dataclass

# Import C++ bindings
from . import rosbag1_py_cpp


@dataclass
class MessageData:
    """Message data structure."""
    topic: str
    data: bytes
    timestamp_ns: int
    type: str
    serialization_format: str
    
    @property
    def timestamp(self) -> rospy.Time:
        """Convert nanoseconds to rospy.Time."""
        secs = self.timestamp_ns // 1_000_000_000
        nsecs = self.timestamp_ns % 1_000_000_000
        return rospy.Time(secs, nsecs)


class Reader:
    """Python wrapper for C++ Reader class."""
    
    def __init__(self):
        self._cpp_reader = rosbag1_py_cpp.Reader()
    
    def open(self, storage_options):
        """
        Open bag for reading.
        
        Args:
            storage_options: Storage configuration (from _writer.StorageOptions)
        """
        self._cpp_reader.open(storage_options.to_cpp())
    
    def close(self):
        """Close bag."""
        self._cpp_reader.close()
    
    def _read_messages_callback(self, callback, topic_filters):
        """Internal method for callback-based reading (not a generator)."""
        # Convert C++ MessageData to Python MessageData in callback wrapper
        def wrapped_callback(cpp_msg):
            msg_data = MessageData(
                topic=cpp_msg.topic,
                data=bytes(cpp_msg.data),
                timestamp_ns=cpp_msg.timestamp_ns,
                type=cpp_msg.type,
                serialization_format=cpp_msg.serialization_format
            )
            callback(msg_data)
        
        self._cpp_reader.read_messages_with_callback(wrapped_callback, topic_filters)
    
    def _read_messages_iterator(self, topic_filters, start_time, end_time):
        """Internal method for iterator-based reading (generator)."""
        # Convert times to nanoseconds
        start_time_ns = 0
        end_time_ns = 2**64 - 1  # UINT64_MAX
        
        if start_time is not None:
            start_time_ns = start_time.secs * 1_000_000_000 + start_time.nsecs
        
        if end_time is not None:
            end_time_ns = end_time.secs * 1_000_000_000 + end_time.nsecs
        
        # Read messages from C++ reader
        cpp_messages = self._cpp_reader.read_messages(
            topic_filters,
            start_time_ns,
            end_time_ns
        )
        
        # Convert to Python MessageData
        for cpp_msg in cpp_messages:
            yield MessageData(
                topic=cpp_msg.topic,
                data=bytes(cpp_msg.data),
                timestamp_ns=cpp_msg.timestamp_ns,
                type=cpp_msg.type,
                serialization_format=cpp_msg.serialization_format
            )
    
    def is_open(self) -> bool:
        """Check if bag is open."""
        return self._cpp_reader.is_open()
    
    def get_topics(self) -> List:
        """Get all topics in the bag."""
        return self._cpp_reader.get_topics()
    
    def read_messages(
        self,
        callback_or_filters: Optional[Callable] = None,
        topic_filters: Optional[List[str]] = None,
        start_time: Optional[rospy.Time] = None,
        end_time: Optional[rospy.Time] = None
    ):
        """
        Read messages from the bag.
        
        Can be used in two ways:
        1. With callback: read_messages(callback, topic_filters=None)
        2. As iterator: read_messages(topic_filters=None, start_time=None, end_time=None)
        
        Args:
            callback_or_filters: If callable, used as callback function. Otherwise treated as topic_filters (for backward compatibility)
            topic_filters: Optional list of topic names to filter (only used if callback_or_filters is not a callable)
            start_time: Optional start time filter
            end_time: Optional end time filter
        
        Yields:
            MessageData objects (if used as iterator)
        """
        if not self.is_open():
            raise RuntimeError("Reader is not open")
        
        # Check if first argument is a callable (callback function)
        if callable(callback_or_filters):
            # Callback-based reading - call non-generator method
            filters = topic_filters or []
            return self._read_messages_callback(callback_or_filters, filters)
        else:
            # Iterator-based reading - call generator method
            filters = callback_or_filters if callback_or_filters is not None else (topic_filters or [])
            return self._read_messages_iterator(filters, start_time, end_time)
    
    def get_metadata(self):
        """Get bag metadata."""
        return self._cpp_reader.get_metadata()

