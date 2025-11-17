"""
Bag compatibility validation.
"""

from typing import List, Dict, Any
from .._reader import Reader
from .._writer import StorageOptions


def validate_bag_compatibility(
    bag_path: str,
    target_ros: int = 2,
    storage_id: str = "mcap"
) -> Dict[str, Any]:
    """
    Validate bag compatibility with target ROS version.
    
    Args:
        bag_path: Path to bag file
        target_ros: Target ROS version (1 or 2)
        storage_id: Storage format
    
    Returns:
        Dictionary with validation results
    """
    reader = Reader()
    storage_opts = StorageOptions(uri=bag_path, storage_id=storage_id)
    
    try:
        reader.open(storage_opts)
        topics = reader.get_topics()
        
        incompatible_topics = []
        compatible_topics = []
        
        from .type_mapping import TypeConverter
        
        for topic in topics:
            if target_ros == 2:
                # Check if type is ROS2 format
                if TypeConverter.is_ros1_type(topic.type):
                    incompatible_topics.append({
                        "name": topic.name,
                        "type": topic.type,
                        "issue": "ROS1 type format, needs conversion"
                    })
                else:
                    compatible_topics.append(topic.name)
            else:
                # Check if type is ROS1 format
                if TypeConverter.is_ros2_type(topic.type):
                    incompatible_topics.append({
                        "name": topic.name,
                        "type": topic.type,
                        "issue": "ROS2 type format, needs conversion"
                    })
                else:
                    compatible_topics.append(topic.name)
        
        return {
            "compatible": len(incompatible_topics) == 0,
            "compatible_topics": len(compatible_topics),
            "incompatible_topics": len(incompatible_topics),
            "incompatible_details": incompatible_topics,
            "total_topics": len(topics)
        }
    finally:
        reader.close()

