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
    
    def set_mcap_options(
        self,
        preset_profile: Optional[str] = None,
        noChunkCRC: Optional[bool] = None,
        noAttachmentCRC: Optional[bool] = None,
        enableDataCRC: Optional[bool] = None,
        noSummaryCRC: Optional[bool] = None,
        noChunking: Optional[bool] = None,
        noMessageIndex: Optional[bool] = None,
        noSummary: Optional[bool] = None,
        noMetadataIndex: Optional[bool] = None,
        noChunkIndex: Optional[bool] = None,
        noStatistics: Optional[bool] = None,
        noSummaryOffsets: Optional[bool] = None,
        forceCompression: Optional[bool] = None,
        chunkSize: Optional[int] = None,
        compression: Optional[str] = None,
        compressionLevel: Optional[int] = None
    ):
        """
        Set MCAP writer options.
        
        Args:
            preset_profile: Preset profile name ("fastwrite", "none", or None for custom)
            noChunkCRC: Disable CRC calculation for Chunks
            noAttachmentCRC: Disable CRC calculation for Attachments
            enableDataCRC: Enable CRC calculation for the entire Data section
            noSummaryCRC: Disable CRC calculation for the Summary section
            noChunking: Do not write Chunks, write records directly into Data section
            noMessageIndex: Do not write Message Index records
            noSummary: Do not write Summary section
            noMetadataIndex: Advanced option
            noChunkIndex: Advanced option
            noStatistics: Advanced option
            noSummaryOffsets: Advanced option
            forceCompression: Force compression even for small chunks
            chunkSize: Chunk size in bytes (default: 786432 = 768KB)
            compression: Compression type ("None", "Lz4", "Zstd")
            compressionLevel: Compression level (-1 for default)
        """
        if preset_profile is not None:
            self.custom_data["mcap_preset_profile"] = preset_profile
        
        if noChunkCRC is not None:
            self.custom_data["mcap_noChunkCRC"] = str(noChunkCRC).lower()
        if noAttachmentCRC is not None:
            self.custom_data["mcap_noAttachmentCRC"] = str(noAttachmentCRC).lower()
        if enableDataCRC is not None:
            self.custom_data["mcap_enableDataCRC"] = str(enableDataCRC).lower()
        if noSummaryCRC is not None:
            self.custom_data["mcap_noSummaryCRC"] = str(noSummaryCRC).lower()
        if noChunking is not None:
            self.custom_data["mcap_noChunking"] = str(noChunking).lower()
        if noMessageIndex is not None:
            self.custom_data["mcap_noMessageIndex"] = str(noMessageIndex).lower()
        if noSummary is not None:
            self.custom_data["mcap_noSummary"] = str(noSummary).lower()
        if noMetadataIndex is not None:
            self.custom_data["mcap_noMetadataIndex"] = str(noMetadataIndex).lower()
        if noChunkIndex is not None:
            self.custom_data["mcap_noChunkIndex"] = str(noChunkIndex).lower()
        if noStatistics is not None:
            self.custom_data["mcap_noStatistics"] = str(noStatistics).lower()
        if noSummaryOffsets is not None:
            self.custom_data["mcap_noSummaryOffsets"] = str(noSummaryOffsets).lower()
        if forceCompression is not None:
            self.custom_data["mcap_forceCompression"] = str(forceCompression).lower()
        
        if chunkSize is not None:
            self.custom_data["mcap_chunkSize"] = str(chunkSize)
        if compression is not None:
            self.custom_data["mcap_compression"] = compression
        if compressionLevel is not None:
            self.custom_data["mcap_compressionLevel"] = str(compressionLevel)
    
    def load_mcap_config_file(self, config_file: str):
        """
        Load MCAP writer options from a YAML config file.
        
        Args:
            config_file: Path to YAML config file
            
        Raises:
            FileNotFoundError: If config file doesn't exist
            ValueError: If YAML parsing fails (when PyYAML is not available)
        """
        import os
        if not os.path.exists(config_file):
            raise FileNotFoundError(f"Config file not found: {config_file}")
        
        try:
            import yaml
            with open(config_file, 'r') as f:
                config = yaml.safe_load(f)
            
            if not isinstance(config, dict):
                raise ValueError("Config file must contain a dictionary")
            
            # Load all options from YAML into custom_data
            for key, value in config.items():
                if isinstance(value, bool):
                    self.custom_data[f"mcap_{key}"] = str(value).lower()
                elif isinstance(value, (int, float)):
                    self.custom_data[f"mcap_{key}"] = str(value)
                elif isinstance(value, str):
                    self.custom_data[f"mcap_{key}"] = value
                else:
                    # Skip non-scalar values
                    continue
                    
        except ImportError:
            # PyYAML not available, try simple key-value parsing
            with open(config_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    if ':' in line:
                        key, value = line.split(':', 1)
                        key = key.strip()
                        value = value.strip().strip('"\'')
                        self.custom_data[f"mcap_{key}"] = value
        except Exception as e:
            raise ValueError(f"Failed to parse config file: {e}")


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

