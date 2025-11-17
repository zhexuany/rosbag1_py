#!/usr/bin/env python3
"""
Basic recording example.
"""

import rospy
from std_msgs.msg import String, Int32
from rosbag1_py import Writer, StorageOptions, TopicMetadata, CompressionOptions


def main():
    rospy.init_node('basic_recording_example')
    
    # Create writer
    writer = Writer()
    
    # Configure storage
    storage_opts = StorageOptions(
        uri='example_bag.mcap',
        storage_id='mcap'
    )
    
    # Optional: Enable compression
    compression_opts = CompressionOptions(
        compression_mode='lz4',
        compression_level=4
    )
    
    # Open bag
    writer.open(storage_opts, compression_options=compression_opts)
    
    # Register topics
    topic1_meta = TopicMetadata(
        id=0,
        name='/chatter',
        type='std_msgs/String',
        serialization_format='cdr'
    )
    writer.create_topic(topic1_meta)
    
    topic2_meta = TopicMetadata(
        id=1,
        name='/numbers',
        type='std_msgs/Int32',
        serialization_format='cdr'
    )
    writer.create_topic(topic2_meta)
    
    # Record some messages
    for i in range(10):
        msg1 = String()
        msg1.data = f"Hello {i}"
        writer.write_message('/chatter', msg1)
        
        msg2 = Int32()
        msg2.data = i
        writer.write_message('/numbers', msg2)
        
        rospy.sleep(0.1)
    
    # Close bag
    writer.close()
    print("Recording complete!")


if __name__ == '__main__':
    main()

