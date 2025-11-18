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

