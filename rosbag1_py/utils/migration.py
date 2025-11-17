"""
Bag migration utilities for ROS1/ROS2 conversion.
"""

from typing import Optional
from .._writer import Writer, StorageOptions, TopicMetadata, ConverterOptions
from .._reader import Reader
from ..compatibility.type_mapping import TypeConverter
import rospy


class BagConverter:
    """Convert bags between ROS versions."""
    
    def __init__(self, input_file: str, output_file: str, target_ros: int):
        """
        Initialize bag converter.
        
        Args:
            input_file: Input bag file path
            output_file: Output bag file path
            target_ros: Target ROS version (1 or 2)
        """
        self.input_file = input_file
        self.output_file = output_file
        self.target_ros = target_ros
        self.source_ros = 2 if target_ros == 1 else 1
    
    def convert(self):
        """Perform conversion"""
        print(f"Converting {self.input_file} from ROS{self.source_ros} to ROS{self.target_ros}")
        
        # Open input bag
        reader = Reader()
        input_storage = StorageOptions(
            uri=self.input_file,
            storage_id='mcap' if self.input_file.endswith('.mcap') else 'rosbag'
        )
        reader.open(input_storage)
        
        # Get metadata
        metadata = reader.get_metadata()
        print(f"Input bag: {metadata.message_count} messages across {len(metadata.topics_with_message_count)} topics")
        
        # Open output bag
        writer = Writer()
        output_storage = StorageOptions(
            uri=self.output_file,
            storage_id='mcap' if self.target_ros == 2 else 'rosbag'
        )
        writer.open(output_storage, ConverterOptions())
        
        # Register topics with converted names
        topic_map = {}
        topic_id = 0
        for topic_name, msg_type, count in metadata.topics_with_message_count:
            converted_type = TypeConverter.ros1_to_ros2(msg_type) if self.target_ros == 2 else TypeConverter.ros2_to_ros1(msg_type)
            
            topic_meta = TopicMetadata(
                id=topic_id,
                name=topic_name,
                type=converted_type,
                serialization_format='cdr'
            )
            writer.create_topic(topic_meta)
            topic_map[topic_name] = converted_type
            topic_id += 1
            
            print(f"  Topic: {topic_name} ({msg_type} -> {converted_type}): {count} messages")
        
        # Convert messages
        message_count = 0
        for msg_data in reader.read_messages():
            # Write converted message
            writer.write_message(msg_data.topic, msg_data.data, msg_data.timestamp)
            message_count += 1
            
            if message_count % 1000 == 0:
                print(f"  Converted {message_count} messages...")
        
        reader.close()
        writer.close()
        
        print(f"Conversion complete: {message_count} messages written to {self.output_file}")


def convert_bag(input_file: str, output_file: str, target_ros: int):
    """
    Convert bag between ROS versions.
    
    Args:
        input_file: Input bag file path
        output_file: Output bag file path
        target_ros: Target ROS version (1 or 2)
    """
    converter = BagConverter(input_file, output_file, target_ros)
    converter.convert()

