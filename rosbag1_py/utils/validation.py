"""
Bag validation utilities.
"""

from typing import Dict, Any
from .._reader import Reader
from .._writer import StorageOptions


def validate_bag(bag_path: str, storage_id: str = "mcap") -> Dict[str, Any]:
    """
    Validate a bag file.
    
    Args:
        bag_path: Path to bag file
        storage_id: Storage format
    
    Returns:
        Dictionary with validation results
    """
    reader = Reader()
    storage_opts = StorageOptions(uri=bag_path, storage_id=storage_id)
    
    try:
        reader.open(storage_opts)
        metadata = reader.get_metadata()
        topics = reader.get_topics()
        
        # Basic validation
        errors = []
        warnings = []
        
        if metadata.message_count == 0:
            warnings.append("Bag contains no messages")
        
        if len(topics) == 0:
            warnings.append("Bag contains no topics")
        
        # Check topic consistency
        for topic in topics:
            if not topic.name.startswith('/'):
                warnings.append(f"Topic name doesn't start with '/': {topic.name}")
        
        return {
            "valid": len(errors) == 0,
            "errors": errors,
            "warnings": warnings,
            "message_count": metadata.message_count,
            "topic_count": len(topics),
            "duration": (metadata.end_time_ns - metadata.start_time_ns) / 1e9
        }
    except Exception as e:
        return {
            "valid": False,
            "errors": [str(e)],
            "warnings": []
        }
    finally:
        reader.close()

