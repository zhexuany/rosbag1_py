#!/usr/bin/env python3
"""
Example demonstrating MCAP writer options configuration.

This example shows different ways to configure MCAP writer options:
1. Using preset profiles
2. Setting individual options programmatically
3. Loading from YAML config file
"""

import rospy
from std_msgs.msg import String, Int32
from rosbag1_py import Writer, StorageOptions, TopicMetadata


def example_preset_profile():
    """Example using preset profile 'fastwrite'."""
    print("=== Example 1: Using preset profile 'fastwrite' ===")
    
    rospy.init_node('mcap_config_example_preset', anonymous=True)
    writer = Writer()
    
    # Create storage options with preset profile
    storage_opts = StorageOptions(
        uri='example_preset.mcap',
        storage_id='mcap'
    )
    # Set preset profile using convenience method
    storage_opts.set_mcap_options(preset_profile='fastwrite')
    
    writer.open(storage_opts)
    
    # Register topic
    topic_meta = TopicMetadata(
        id=0,
        name='/chatter',
        type='std_msgs/String',
        serialization_format='cdr'
    )
    writer.create_topic(topic_meta)
    
    # Write some messages
    for i in range(5):
        msg = String()
        msg.data = f"Preset example message {i}"
        writer.write_message('/chatter', msg)
        rospy.sleep(0.1)
    
    writer.close()
    print("Recorded with 'fastwrite' preset profile\n")


def example_custom_options():
    """Example setting individual MCAP options."""
    print("=== Example 2: Setting custom MCAP options ===")
    
    rospy.init_node('mcap_config_example_custom', anonymous=True)
    writer = Writer()
    
    storage_opts = StorageOptions(
        uri='example_custom.mcap',
        storage_id='mcap'
    )
    
    # Set custom options
    storage_opts.set_mcap_options(
        compression='Lz4',
        chunkSize=2048,  # 2KB chunks
        noChunkCRC=True,
        compressionLevel=5
    )
    
    writer.open(storage_opts)
    
    topic_meta = TopicMetadata(
        id=0,
        name='/numbers',
        type='std_msgs/Int32',
        serialization_format='cdr'
    )
    writer.create_topic(topic_meta)
    
    for i in range(5):
        msg = Int32()
        msg.data = i * 10
        writer.write_message('/numbers', msg)
        rospy.sleep(0.1)
    
    writer.close()
    print("Recorded with custom MCAP options\n")


def example_yaml_config():
    """Example loading options from YAML config file."""
    print("=== Example 3: Loading from YAML config file ===")
    
    import os
    import tempfile
    
    # Create a temporary YAML config file
    config_content = """# MCAP writer options
noChunkCRC: false
noChunking: false
noMessageIndex: false
noSummary: false
chunkSize: 4096
compression: Zstd
compressionLevel: 3
"""
    
    # Write config to temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yml', delete=False) as f:
        f.write(config_content)
        config_file = f.name
    
    try:
        rospy.init_node('mcap_config_example_yaml', anonymous=True)
        writer = Writer()
        
        storage_opts = StorageOptions(
            uri='example_yaml.mcap',
            storage_id='mcap'
        )
        
        # Load options from YAML file
        storage_opts.load_mcap_config_file(config_file)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/chatter',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        for i in range(5):
            msg = String()
            msg.data = f"YAML config message {i}"
            writer.write_message('/chatter', msg)
            rospy.sleep(0.1)
        
        writer.close()
        print("Recorded with options from YAML config file\n")
    finally:
        # Clean up temporary file
        if os.path.exists(config_file):
            os.unlink(config_file)


def example_direct_custom_data():
    """Example using custom_data directly."""
    print("=== Example 4: Using custom_data directly ===")
    
    rospy.init_node('mcap_config_example_direct', anonymous=True)
    writer = Writer()
    
    # Set options directly in custom_data
    storage_opts = StorageOptions(
        uri='example_direct.mcap',
        storage_id='mcap',
        custom_data={
            'mcap_preset_profile': 'fastwrite',
            'mcap_compression': 'None',
            'mcap_chunkSize': '1024'
        }
    )
    
    writer.open(storage_opts)
    
    topic_meta = TopicMetadata(
        id=0,
        name='/chatter',
        type='std_msgs/String',
        serialization_format='cdr'
    )
    writer.create_topic(topic_meta)
    
    for i in range(5):
        msg = String()
        msg.data = f"Direct custom_data message {i}"
        writer.write_message('/chatter', msg)
        rospy.sleep(0.1)
    
    writer.close()
    print("Recorded with options set via custom_data\n")


def main():
    """Run all examples."""
    print("MCAP Writer Options Configuration Examples\n")
    print("=" * 50 + "\n")
    
    try:
        example_preset_profile()
        example_custom_options()
        example_yaml_config()
        example_direct_custom_data()
        
        print("=" * 50)
        print("All examples completed successfully!")
        print("\nGenerated files:")
        print("  - example_preset.mcap")
        print("  - example_custom.mcap")
        print("  - example_yaml.mcap")
        print("  - example_direct.mcap")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == '__main__':
    main()

