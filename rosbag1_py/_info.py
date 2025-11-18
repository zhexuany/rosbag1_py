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
Bag info utilities.
"""

from typing import Dict, Any
from ._reader import Reader
from ._writer import StorageOptions


def get_bag_info(bag_path: str, storage_id: str = "mcap") -> Dict[str, Any]:
    """
    Get information about a bag file.
    
    Args:
        bag_path: Path to bag file
        storage_id: Storage format ("mcap" or "rosbag")
    
    Returns:
        Dictionary with bag information
    """
    reader = Reader()
    storage_opts = StorageOptions(uri=bag_path, storage_id=storage_id)
    
    try:
        reader.open(storage_opts)
        metadata = reader.get_metadata()
        
        info = {
            "path": bag_path,
            "storage_id": storage_id,
            "message_count": metadata.message_count,
            "start_time": metadata.start_time_ns,
            "end_time": metadata.end_time_ns,
            "duration": (metadata.end_time_ns - metadata.start_time_ns) / 1e9,
            "topics": []
        }
        
        for topic_name, topic_type, msg_count in metadata.topics_with_message_count:
            info["topics"].append({
                "name": topic_name,
                "type": topic_type,
                "message_count": msg_count
            })
        
        return info
    finally:
        reader.close()

