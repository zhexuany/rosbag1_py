"""
Python wrapper for C++ Writer class.
"""

import rospy
from typing import Optional, Dict, Any
import time

# Import C++ bindings
from . import rosbag1_py_cpp


class StorageOptions:
    """Storage configuration options."""
    
    def __init__(
        self,
        uri: str,
        storage_id: str = "mcap",
        append: bool = False,
        custom_data: Optional[Dict[str, str]] = None
    ):
        self.uri = uri
        self.storage_id = self._parse_storage_id(storage_id)
        self.append = append
        self.custom_data = custom_data or {}
    
    def _parse_storage_id(self, storage_id: str):
        """Parse storage ID string to enum."""
        storage_id_lower = storage_id.lower()
        if storage_id_lower == "mcap":
            return rosbag1_py_cpp.StorageFormat.MCAP
        elif storage_id_lower == "rosbag":
            return rosbag1_py_cpp.StorageFormat.ROSBAG
        elif storage_id_lower == "sqlite":
            return rosbag1_py_cpp.StorageFormat.SQLITE
        else:
            raise ValueError(f"Unknown storage_id: {storage_id}")
    
    def to_cpp(self) -> rosbag1_py_cpp.StorageOptions:
        """Convert to C++ StorageOptions."""
        cpp_opts = rosbag1_py_cpp.StorageOptions()
        cpp_opts.uri = self.uri
        cpp_opts.storage_id = self.storage_id
        cpp_opts.append = self.append
        cpp_opts.custom_data = self.custom_data
        return cpp_opts


class ConverterOptions:
    """Converter configuration options."""
    
    def __init__(
        self,
        input_serialization_format: str = "cdr",
        output_serialization_format: str = "cdr"
    ):
        self.input_serialization_format = input_serialization_format
        self.output_serialization_format = output_serialization_format
    
    def to_cpp(self) -> rosbag1_py_cpp.ConverterOptions:
        """Convert to C++ ConverterOptions."""
        cpp_opts = rosbag1_py_cpp.ConverterOptions()
        cpp_opts.input_serialization_format = self.input_serialization_format
        cpp_opts.output_serialization_format = self.output_serialization_format
        return cpp_opts


class CompressionOptions:
    """Compression configuration options."""
    
    def __init__(
        self,
        compression_mode: str = "none",
        compression_level: int = -1,
        compression_queue_size: int = 1000,
        compression_threads: int = 4
    ):
        self.compression_mode = self._parse_compression_mode(compression_mode)
        self.compression_level = compression_level
        self.compression_queue_size = compression_queue_size
        self.compression_threads = compression_threads
    
    def _parse_compression_mode(self, mode: str):
        """Parse compression mode string to enum."""
        mode_lower = mode.lower()
        if mode_lower == "none":
            return rosbag1_py_cpp.CompressionMode.NONE
        elif mode_lower == "bz2":
            return rosbag1_py_cpp.CompressionMode.BZ2
        elif mode_lower == "lz4":
            return rosbag1_py_cpp.CompressionMode.LZ4
        elif mode_lower == "zstd":
            return rosbag1_py_cpp.CompressionMode.ZSTD
        elif mode_lower == "gzip":
            return rosbag1_py_cpp.CompressionMode.GZIP
        else:
            raise ValueError(f"Unknown compression_mode: {mode}")
    
    def to_cpp(self) -> rosbag1_py_cpp.CompressionOptions:
        """Convert to C++ CompressionOptions."""
        cpp_opts = rosbag1_py_cpp.CompressionOptions()
        cpp_opts.compression_mode = self.compression_mode
        cpp_opts.compression_level = self.compression_level
        cpp_opts.compression_queue_size = self.compression_queue_size
        cpp_opts.compression_threads = self.compression_threads
        return cpp_opts


# Export compression mode enum
CompressionMode = rosbag1_py_cpp.CompressionMode
StorageFormat = rosbag1_py_cpp.StorageFormat


class TopicMetadata:
    """Topic metadata."""
    
    def __init__(
        self,
        id: int,
        name: str,
        type: str,
        serialization_format: str = "cdr",
        md5sum: str = "",
        definition: str = "",
        metadata: Optional[Dict[str, str]] = None
    ):
        self.id = id
        self.name = name
        self.type = type
        self.serialization_format = serialization_format
        self.md5sum = md5sum
        self.definition = definition
        self.metadata = metadata or {}
    
    def to_cpp(self) -> rosbag1_py_cpp.TopicMetadata:
        """Convert to C++ TopicMetadata."""
        cpp_meta = rosbag1_py_cpp.TopicMetadata()
        cpp_meta.id = self.id
        cpp_meta.name = self.name
        cpp_meta.type = self.type
        cpp_meta.serialization_format = self.serialization_format
        cpp_meta.md5sum = self.md5sum
        cpp_meta.definition = self.definition
        cpp_meta.metadata = self.metadata
        return cpp_meta


class SplitOptions:
    """Bag splitting configuration options."""
    
    def __init__(
        self,
        mode: str = "size",
        max_size: int = 1024 * 1024 * 1024,  # 1GB
        max_duration: float = 300.0,  # 5 minutes
        max_messages: int = 100000,
        naming_pattern: str = "{basename}_{index:03d}"
    ):
        self.mode = self._parse_split_mode(mode)
        self.max_size = max_size
        self.max_duration = max_duration
        self.max_messages = max_messages
        self.naming_pattern = naming_pattern
    
    def _parse_split_mode(self, mode: str):
        """Parse split mode string to enum."""
        mode_lower = mode.lower()
        if mode_lower == "size":
            return rosbag1_py_cpp.SplitMode.SIZE
        elif mode_lower == "duration":
            return rosbag1_py_cpp.SplitMode.DURATION
        elif mode_lower == "message_count":
            return rosbag1_py_cpp.SplitMode.MESSAGE_COUNT
        else:
            raise ValueError(f"Unknown split mode: {mode}")
    
    def to_cpp(self) -> rosbag1_py_cpp.SplitOptions:
        """Convert to C++ SplitOptions."""
        cpp_opts = rosbag1_py_cpp.SplitOptions()
        cpp_opts.mode = self.mode
        cpp_opts.max_size = self.max_size
        cpp_opts.max_duration = self.max_duration
        cpp_opts.max_messages = self.max_messages
        cpp_opts.naming_pattern = self.naming_pattern
        return cpp_opts


class Writer:
    """Python wrapper for C++ Writer class."""
    
    def __init__(self):
        self._cpp_writer = rosbag1_py_cpp.Writer()
        self._topics = {}
    
    def open(
        self,
        storage_options: StorageOptions,
        converter_options: Optional[ConverterOptions] = None,
        compression_options: Optional[CompressionOptions] = None,
        split_options: Optional[SplitOptions] = None
    ):
        """
        Open bag for writing.
        
        Args:
            storage_options: Storage configuration
            converter_options: Optional converter configuration
            compression_options: Optional compression configuration
            split_options: Optional bag splitting configuration
        """
        conv_opts = converter_options or ConverterOptions()
        comp_opts = compression_options or CompressionOptions()
        split_opts = split_options.to_cpp() if split_options else rosbag1_py_cpp.SplitOptions()
        
        self._cpp_writer.open(
            storage_options.to_cpp(),
            conv_opts.to_cpp(),
            comp_opts.to_cpp(),
            split_opts
        )
    
    def close(self):
        """Close bag."""
        self._cpp_writer.close()
    
    def is_open(self) -> bool:
        """Check if bag is open."""
        return self._cpp_writer.is_open()
    
    def create_topic(self, topic_metadata: TopicMetadata):
        """
        Register a topic.
        
        Args:
            topic_metadata: Topic metadata
        """
        self._topics[topic_metadata.name] = topic_metadata
        self._cpp_writer.create_topic(topic_metadata.to_cpp())
    
    def write_message(
        self,
        topic: str,
        msg,
        timestamp: Optional[rospy.Time] = None
    ):
        """
        Write a message to the bag.
        
        Args:
            topic: Topic name
            msg: ROS message object
            timestamp: Optional timestamp (defaults to current time)
        """
        if not self.is_open():
            raise RuntimeError("Writer is not open")
        
        if topic not in self._topics:
            raise ValueError(f"Topic not registered: {topic}")
        
        # Get timestamp
        if timestamp is None:
            timestamp = rospy.Time.now()
        
        # Serialize message using ROS1 serialization
        # ROS1 messages have a serialize() method that takes a buffer
        import io
        buff = io.BytesIO()
        msg.serialize(buff)
        serialized = buff.getvalue()
        
        # Convert timestamp to nanoseconds
        timestamp_ns = timestamp.secs * 1_000_000_000 + timestamp.nsecs
        
        # Write to C++ writer
        # Pass bytes directly - pybind11 will handle conversion
        self._cpp_writer.write_message(
            topic,
            bytes(serialized),  # Ensure it's bytes
            len(serialized),
            timestamp_ns
        )
    
    def get_metadata(self) -> Dict[str, str]:
        """Get bag metadata."""
        return self._cpp_writer.get_metadata()

