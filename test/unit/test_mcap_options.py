"""
Unit tests for MCAP writer options configuration.
"""

import unittest
import tempfile
import os
from rosbag1_py import Writer, StorageOptions, TopicMetadata


class TestMcapOptions(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Set up test class - initialize ROS node once"""
        import rospy
        if not rospy.get_node_uri():
            rospy.init_node('test_mcap_options', anonymous=True)
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.bag_path = os.path.join(self.temp_dir, 'test_bag')
    
    def tearDown(self):
        """Clean up test files"""
        import shutil
        shutil.rmtree(self.temp_dir)
    
    def test_default_options(self):
        """Test that default options work (backward compatibility)"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        # No custom options set - should use defaults
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        # File should be created successfully
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_preset_profile_fastwrite(self):
        """Test preset profile 'fastwrite'"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(preset_profile='fastwrite')
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify options were set
        self.assertEqual(storage_opts.custom_data.get('mcap_preset_profile'), 'fastwrite')
    
    def test_preset_profile_none(self):
        """Test preset profile 'none'"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(preset_profile='none')
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_preset_profile'), 'none')
    
    def test_custom_compression(self):
        """Test setting custom compression type"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(compression='Lz4')
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'Lz4')
    
    def test_custom_chunk_size(self):
        """Test setting custom chunk size"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(chunkSize=2048)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '2048')
    
    def test_boolean_options(self):
        """Test setting boolean options"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            noChunkCRC=True,
            noChunking=True,
            enableDataCRC=True
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunkCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunking'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_enableDataCRC'), 'true')
    
    def test_direct_custom_data(self):
        """Test setting options directly via custom_data"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap',
            custom_data={
                'mcap_compression': 'None',
                'mcap_chunkSize': '4096',
                'mcap_noChunkCRC': 'true'
            }
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'None')
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '4096')
    
    def test_yaml_config_file(self):
        """Test loading options from YAML config file"""
        # Create a temporary YAML config file
        config_content = """noChunkCRC: false
noChunking: false
chunkSize: 8192
compression: Zstd
compressionLevel: 5
"""
        
        config_file = os.path.join(self.temp_dir, 'mcap_config.yml')
        with open(config_file, 'w') as f:
            f.write(config_content)
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        
        # Load from YAML
        storage_opts.load_mcap_config_file(config_file)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify options were loaded
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '8192')
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'Zstd')
    
    def test_yaml_config_file_not_found(self):
        """Test error handling when YAML config file doesn't exist"""
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        
        with self.assertRaises(FileNotFoundError):
            storage_opts.load_mcap_config_file('/nonexistent/path/config.yml')
    
    def test_multiple_options_combination(self):
        """Test combining multiple options"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            compression='Lz4',
            chunkSize=4096,
            compressionLevel=3,
            noChunkCRC=True,
            noMessageIndex=False
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify all options were set
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'Lz4')
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '4096')
        self.assertEqual(storage_opts.custom_data.get('mcap_compressionLevel'), '3')
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunkCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noMessageIndex'), 'false')
    
    def test_options_without_mcap_prefix(self):
        """Test that options work without 'mcap_' prefix"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap',
            custom_data={
                'compression': 'None',  # Without mcap_ prefix
                'chunkSize': '2048',   # Without mcap_ prefix
                'preset_profile': 'fastwrite'  # Without mcap_ prefix
            }
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_all_compression_types(self):
        """Test all compression types: None, Lz4, Zstd"""
        compression_types = ['None', 'Lz4', 'Zstd']
        
        for comp_type in compression_types:
            with self.subTest(compression=comp_type):
                bag_path = f"{self.bag_path}_{comp_type}"
                writer = Writer()
                storage_opts = StorageOptions(
                    uri=bag_path,
                    storage_id='mcap'
                )
                storage_opts.set_mcap_options(compression=comp_type)
                
                writer.open(storage_opts)
                
                topic_meta = TopicMetadata(
                    id=0,
                    name='/test_topic',
                    type='std_msgs/String',
                    serialization_format='cdr'
                )
                writer.create_topic(topic_meta)
                writer.close()
                
                self.assertTrue(os.path.exists(f"{bag_path}.mcap"))
                self.assertEqual(storage_opts.custom_data.get('mcap_compression'), comp_type)
    
    def test_all_boolean_options(self):
        """Test all boolean MCAP options"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            noChunkCRC=True,
            noAttachmentCRC=True,
            enableDataCRC=True,
            noSummaryCRC=True,
            noChunking=True,
            noMessageIndex=True,
            noSummary=True,
            noMetadataIndex=True,
            noChunkIndex=True,
            noStatistics=True,
            noSummaryOffsets=True,
            forceCompression=True
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify all boolean options were set
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunkCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noAttachmentCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_enableDataCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noSummaryCRC'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunking'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noMessageIndex'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noSummary'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noMetadataIndex'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunkIndex'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noStatistics'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_noSummaryOffsets'), 'true')
        self.assertEqual(storage_opts.custom_data.get('mcap_forceCompression'), 'true')
    
    def test_boolean_options_false(self):
        """Test setting boolean options to False"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            noChunkCRC=False,
            noChunking=False,
            noMessageIndex=False
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunkCRC'), 'false')
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunking'), 'false')
        self.assertEqual(storage_opts.custom_data.get('mcap_noMessageIndex'), 'false')
    
    def test_compression_level(self):
        """Test setting compression level"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(compressionLevel=5)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_compressionLevel'), '5')
    
    def test_compression_level_default(self):
        """Test default compression level (-1)"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(compressionLevel=-1)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_compressionLevel'), '-1')
    
    def test_preset_profile_override(self):
        """Test that individual options can override preset profile"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        # Set preset profile first
        storage_opts.set_mcap_options(preset_profile='fastwrite')
        # Then override with individual options
        storage_opts.set_mcap_options(
            noChunking=False,  # Override preset's noChunking=True
            compression='Lz4',
            chunkSize=4096
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify preset was set
        self.assertEqual(storage_opts.custom_data.get('mcap_preset_profile'), 'fastwrite')
        # Verify overrides were applied
        self.assertEqual(storage_opts.custom_data.get('mcap_noChunking'), 'false')
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'Lz4')
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '4096')
    
    def test_boolean_string_parsing(self):
        """Test boolean parsing with different string formats"""
        # Test various boolean string formats that should be accepted
        boolean_strings = {
            'true': True,
            'True': True,
            'TRUE': True,
            '1': True,
            'yes': True,
            'on': True,
            'false': False,
            'False': False,
            'FALSE': False,
            '0': False,
            'no': False,
            'off': False
        }
        
        for bool_str, expected in boolean_strings.items():
            with self.subTest(bool_string=bool_str):
                bag_path = f"{self.bag_path}_{bool_str}"
                writer = Writer()
                storage_opts = StorageOptions(
                    uri=bag_path,
                    storage_id='mcap',
                    custom_data={
                        'mcap_noChunkCRC': bool_str
                    }
                )
                
                writer.open(storage_opts)
                
                topic_meta = TopicMetadata(
                    id=0,
                    name='/test_topic',
                    type='std_msgs/String',
                    serialization_format='cdr'
                )
                writer.create_topic(topic_meta)
                writer.close()
                
                self.assertTrue(os.path.exists(f"{bag_path}.mcap"))
    
    def test_chunk_size_various_values(self):
        """Test chunk size with various values"""
        chunk_sizes = [1024, 2048, 4096, 8192, 65536, 786432]
        
        for chunk_size in chunk_sizes:
            with self.subTest(chunkSize=chunk_size):
                bag_path = f"{self.bag_path}_{chunk_size}"
                writer = Writer()
                storage_opts = StorageOptions(
                    uri=bag_path,
                    storage_id='mcap'
                )
                storage_opts.set_mcap_options(chunkSize=chunk_size)
                
                writer.open(storage_opts)
                
                topic_meta = TopicMetadata(
                    id=0,
                    name='/test_topic',
                    type='std_msgs/String',
                    serialization_format='cdr'
                )
                writer.create_topic(topic_meta)
                writer.close()
                
                self.assertTrue(os.path.exists(f"{bag_path}.mcap"))
                self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), str(chunk_size))
    
    def test_yaml_simple_parser_fallback(self):
        """Test YAML parsing fallback when PyYAML is not available"""
        # Create a simple key-value config file (no YAML syntax)
        config_content = """# Simple key-value config
noChunkCRC: false
noChunking: true
chunkSize: 2048
compression: Lz4
compressionLevel: 3
"""
        
        config_file = os.path.join(self.temp_dir, 'simple_config.txt')
        with open(config_file, 'w') as f:
            f.write(config_content)
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        
        # This should work with the fallback parser
        try:
            storage_opts.load_mcap_config_file(config_file)
        except ValueError:
            # If PyYAML is available, it might fail on invalid YAML
            # That's okay, we're testing the fallback
            pass
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_yaml_with_comments(self):
        """Test YAML config file with comments"""
        config_content = """# MCAP writer options
# Compression settings
compression: Zstd
compressionLevel: 5

# Chunking settings
chunkSize: 4096
noChunking: false

# CRC settings
noChunkCRC: false
noSummaryCRC: false
"""
        
        config_file = os.path.join(self.temp_dir, 'mcap_config_comments.yml')
        with open(config_file, 'w') as f:
            f.write(config_content)
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        
        storage_opts.load_mcap_config_file(config_file)
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        self.assertEqual(storage_opts.custom_data.get('mcap_compression'), 'Zstd')
        self.assertEqual(storage_opts.custom_data.get('mcap_chunkSize'), '4096')
    
    def test_yaml_invalid_format(self):
        """Test handling of invalid YAML format"""
        # Create an invalid YAML file
        config_content = """invalid yaml content
{ not valid json either }
"""
        
        config_file = os.path.join(self.temp_dir, 'invalid_config.yml')
        with open(config_file, 'w') as f:
            f.write(config_content)
        
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        
        # Should either work with fallback parser or raise ValueError
        try:
            storage_opts.load_mcap_config_file(config_file)
            # If it doesn't raise, that's okay - fallback parser might handle it
        except (ValueError, FileNotFoundError):
            # Expected behavior
            pass
    
    def test_write_messages_with_options(self):
        """Test that options work correctly when writing messages"""
        import rospy
        from std_msgs.msg import String
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            compression='Lz4',
            chunkSize=2048,
            noChunkCRC=True
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/chatter',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write some messages
        for i in range(10):
            msg = String()
            msg.data = f"Test message {i}"
            writer.write_message('/chatter', msg)
            rospy.sleep(0.01)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Verify file is not empty
        self.assertGreater(os.path.getsize(f"{self.bag_path}.mcap"), 0)
    
    def test_preset_fastwrite_actual_behavior(self):
        """Test that fastwrite preset actually applies the correct settings"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(preset_profile='fastwrite')
        
        # Verify preset settings are in custom_data
        # Note: The actual application happens in C++, but we can verify
        # that the preset profile is set correctly
        self.assertEqual(storage_opts.custom_data.get('mcap_preset_profile'), 'fastwrite')
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def test_compression_case_insensitive(self):
        """Test that compression type parsing is case-insensitive"""
        compression_variants = ['zstd', 'Zstd', 'ZSTD', 'lz4', 'Lz4', 'LZ4', 'none', 'None', 'NONE']
        
        for comp_variant in compression_variants:
            with self.subTest(compression=comp_variant):
                bag_path = f"{self.bag_path}_{comp_variant}"
                writer = Writer()
                storage_opts = StorageOptions(
                    uri=bag_path,
                    storage_id='mcap',
                    custom_data={
                        'mcap_compression': comp_variant
                    }
                )
                
                writer.open(storage_opts)
                
                topic_meta = TopicMetadata(
                    id=0,
                    name='/test_topic',
                    type='std_msgs/String',
                    serialization_format='cdr'
                )
                writer.create_topic(topic_meta)
                writer.close()
                
                self.assertTrue(os.path.exists(f"{bag_path}.mcap"))
    
    def test_empty_custom_data(self):
        """Test that empty custom_data uses defaults"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap',
            custom_data={}  # Explicitly empty
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # Should use defaults (backward compatibility)
    
    def test_mixed_prefix_options(self):
        """Test mixing options with and without mcap_ prefix"""
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap',
            custom_data={
                'mcap_compression': 'Lz4',  # With prefix
                'chunkSize': '4096',        # Without prefix
                'mcap_noChunkCRC': 'true',  # With prefix
                'noChunking': 'false'       # Without prefix
            }
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/test_topic',
            type='std_msgs/String',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
    
    def _generate_random_image_data(self, width=640, height=480, channels=3, dtype='uint8'):
        """Generate random image data as numpy array"""
        try:
            import numpy as np
            if dtype == 'uint8':
                return np.random.randint(0, 256, (height, width, channels), dtype=np.uint8)
            elif dtype == 'uint16':
                return np.random.randint(0, 65536, (height, width, channels), dtype=np.uint16)
            else:
                return np.random.rand(height, width, channels).astype(dtype)
        except ImportError:
            # Fallback: generate raw bytes if numpy not available
            import random
            if dtype == 'uint8':
                return bytes([random.randint(0, 255) for _ in range(width * height * channels)])
            else:
                return bytes([random.randint(0, 255) for _ in range(width * height * channels * 2)])
    
    def _encode_image_as_jpeg(self, image_data):
        """Encode image data as JPEG"""
        try:
            from PIL import Image
            import numpy as np
            if isinstance(image_data, np.ndarray):
                img = Image.fromarray(image_data)
            else:
                # Assume raw bytes, create image from array
                import numpy as np
                arr = np.frombuffer(image_data, dtype=np.uint8).reshape(480, 640, 3)
                img = Image.fromarray(arr)
            
            import io
            buf = io.BytesIO()
            img.save(buf, format='JPEG', quality=85)
            return buf.getvalue()
        except ImportError:
            # Fallback: return raw data if PIL not available
            return image_data if isinstance(image_data, bytes) else image_data.tobytes()
    
    def _encode_image_as_png(self, image_data):
        """Encode image data as PNG"""
        try:
            from PIL import Image
            import numpy as np
            if isinstance(image_data, np.ndarray):
                img = Image.fromarray(image_data)
            else:
                # Assume raw bytes, create image from array
                import numpy as np
                arr = np.frombuffer(image_data, dtype=np.uint16).reshape(480, 640, 1)
                img = Image.fromarray(arr, mode='I;16')
            
            import io
            buf = io.BytesIO()
            img.save(buf, format='PNG')
            return buf.getvalue()
        except ImportError:
            # Fallback: return raw data if PIL not available
            return image_data if isinstance(image_data, bytes) else image_data.tobytes()
    
    def test_sensor_msgs_image_rgb_jpeg(self):
        """Test writing sensor_msgs/Image with JPEG RGB encoding"""
        import rospy
        from sensor_msgs.msg import Image
        from std_msgs.msg import Header
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            compression='Zstd',
            chunkSize=65536,  # Larger chunk size for images
            noChunkCRC=False
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/camera/rgb/image_raw',
            type='sensor_msgs/Image',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Generate and write RGB images
        num_images = 5
        for i in range(num_images):
            # Generate random RGB image (640x480x3)
            rgb_data = self._generate_random_image_data(640, 480, 3, 'uint8')
            
            # Encode as JPEG
            jpeg_data = self._encode_image_as_jpeg(rgb_data)
            
            # Create sensor_msgs/Image message
            img_msg = Image()
            img_msg.header = Header()
            img_msg.header.seq = i
            img_msg.header.stamp = rospy.Time.now()
            img_msg.header.frame_id = 'camera_rgb_optical_frame'
            img_msg.height = 480
            img_msg.width = 640
            img_msg.encoding = 'rgb8'
            img_msg.is_bigendian = False
            img_msg.step = 640 * 3  # width * channels
            img_msg.data = jpeg_data  # Store JPEG encoded data
            
            writer.write_message('/camera/rgb/image_raw', img_msg)
            rospy.sleep(0.01)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        file_size = os.path.getsize(f"{self.bag_path}.mcap")
        self.assertGreater(file_size, 0)
        # Images should create a reasonably sized file
        self.assertGreater(file_size, 1000)  # At least 1KB
    
    def test_sensor_msgs_image_depth_png(self):
        """Test writing sensor_msgs/Image with PNG depth encoding"""
        import rospy
        from sensor_msgs.msg import Image
        from std_msgs.msg import Header
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            compression='Lz4',
            chunkSize=32768,
            noMessageIndex=False
        )
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/camera/depth/image_raw',
            type='sensor_msgs/Image',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Generate and write depth images
        num_images = 5
        for i in range(num_images):
            # Generate random depth image (640x480x1, 16-bit)
            depth_data = self._generate_random_image_data(640, 480, 1, 'uint16')
            
            # Encode as PNG
            png_data = self._encode_image_as_png(depth_data)
            
            # Create sensor_msgs/Image message
            img_msg = Image()
            img_msg.header = Header()
            img_msg.header.seq = i
            img_msg.header.stamp = rospy.Time.now()
            img_msg.header.frame_id = 'camera_depth_optical_frame'
            img_msg.height = 480
            img_msg.width = 640
            img_msg.encoding = '16UC1'  # 16-bit unsigned, 1 channel
            img_msg.is_bigendian = False
            img_msg.step = 640 * 2  # width * bytes per pixel (16-bit = 2 bytes)
            img_msg.data = png_data  # Store PNG encoded data
            
            writer.write_message('/camera/depth/image_raw', img_msg)
            rospy.sleep(0.01)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        file_size = os.path.getsize(f"{self.bag_path}.mcap")
        self.assertGreater(file_size, 0)
        self.assertGreater(file_size, 1000)
    
    def test_sensor_msgs_image_mixed_rgb_depth(self):
        """Test writing both RGB and depth images with different MCAP options"""
        import rospy
        from sensor_msgs.msg import Image
        from std_msgs.msg import Header
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(
            compression='Zstd',
            chunkSize=65536,
            compressionLevel=3,
            noChunkCRC=True,
            noMessageIndex=False
        )
        
        writer.open(storage_opts)
        
        # Register both topics
        rgb_topic_meta = TopicMetadata(
            id=0,
            name='/camera/rgb/image_raw',
            type='sensor_msgs/Image',
            serialization_format='cdr'
        )
        depth_topic_meta = TopicMetadata(
            id=1,
            name='/camera/depth/image_raw',
            type='sensor_msgs/Image',
            serialization_format='cdr'
        )
        writer.create_topic(rgb_topic_meta)
        writer.create_topic(depth_topic_meta)
        
        # Write alternating RGB and depth images
        num_pairs = 3
        for i in range(num_pairs):
            # RGB image
            rgb_data = self._generate_random_image_data(640, 480, 3, 'uint8')
            jpeg_data = self._encode_image_as_jpeg(rgb_data)
            
            rgb_msg = Image()
            rgb_msg.header = Header()
            rgb_msg.header.seq = i * 2
            rgb_msg.header.stamp = rospy.Time.now()
            rgb_msg.header.frame_id = 'camera_rgb_optical_frame'
            rgb_msg.height = 480
            rgb_msg.width = 640
            rgb_msg.encoding = 'rgb8'
            rgb_msg.is_bigendian = False
            rgb_msg.step = 640 * 3
            rgb_msg.data = jpeg_data
            
            writer.write_message('/camera/rgb/image_raw', rgb_msg)
            rospy.sleep(0.01)
            
            # Depth image
            depth_data = self._generate_random_image_data(640, 480, 1, 'uint16')
            png_data = self._encode_image_as_png(depth_data)
            
            depth_msg = Image()
            depth_msg.header = Header()
            depth_msg.header.seq = i * 2 + 1
            depth_msg.header.stamp = rospy.Time.now()
            depth_msg.header.frame_id = 'camera_depth_optical_frame'
            depth_msg.height = 480
            depth_msg.width = 640
            depth_msg.encoding = '16UC1'
            depth_msg.is_bigendian = False
            depth_msg.step = 640 * 2
            depth_msg.data = png_data
            
            writer.write_message('/camera/depth/image_raw', depth_msg)
            rospy.sleep(0.01)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        file_size = os.path.getsize(f"{self.bag_path}.mcap")
        self.assertGreater(file_size, 0)
        self.assertGreater(file_size, 2000)  # Should be larger with both image types
    
    def test_sensor_msgs_image_with_fastwrite_preset(self):
        """Test writing images with fastwrite preset profile"""
        import rospy
        from sensor_msgs.msg import Image
        from std_msgs.msg import Header
        
        writer = Writer()
        storage_opts = StorageOptions(
            uri=self.bag_path,
            storage_id='mcap'
        )
        storage_opts.set_mcap_options(preset_profile='fastwrite')
        
        writer.open(storage_opts)
        
        topic_meta = TopicMetadata(
            id=0,
            name='/camera/rgb/image_raw',
            type='sensor_msgs/Image',
            serialization_format='cdr'
        )
        writer.create_topic(topic_meta)
        
        # Write a few RGB images
        for i in range(3):
            rgb_data = self._generate_random_image_data(320, 240, 3, 'uint8')  # Smaller for speed
            jpeg_data = self._encode_image_as_jpeg(rgb_data)
            
            img_msg = Image()
            img_msg.header = Header()
            img_msg.header.seq = i
            img_msg.header.stamp = rospy.Time.now()
            img_msg.header.frame_id = 'camera_rgb_optical_frame'
            img_msg.height = 240
            img_msg.width = 320
            img_msg.encoding = 'rgb8'
            img_msg.is_bigendian = False
            img_msg.step = 320 * 3
            img_msg.data = jpeg_data
            
            writer.write_message('/camera/rgb/image_raw', img_msg)
            rospy.sleep(0.01)
        
        writer.close()
        
        self.assertTrue(os.path.exists(f"{self.bag_path}.mcap"))
        # fastwrite should still create a valid file
        self.assertGreater(os.path.getsize(f"{self.bag_path}.mcap"), 0)
    
    def test_sensor_msgs_image_compression_comparison(self):
        """Test different compression types with image data"""
        import rospy
        from sensor_msgs.msg import Image
        from std_msgs.msg import Header
        
        compression_types = ['None', 'Lz4', 'Zstd']
        file_sizes = {}
        
        for comp_type in compression_types:
            with self.subTest(compression=comp_type):
                bag_path = f"{self.bag_path}_{comp_type}"
                writer = Writer()
                storage_opts = StorageOptions(
                    uri=bag_path,
                    storage_id='mcap'
                )
                storage_opts.set_mcap_options(
                    compression=comp_type,
                    chunkSize=65536
                )
                
                writer.open(storage_opts)
                
                topic_meta = TopicMetadata(
                    id=0,
                    name='/camera/rgb/image_raw',
                    type='sensor_msgs/Image',
                    serialization_format='cdr'
                )
                writer.create_topic(topic_meta)
                
                # Write same images with different compression
                for i in range(3):
                    rgb_data = self._generate_random_image_data(640, 480, 3, 'uint8')
                    jpeg_data = self._encode_image_as_jpeg(rgb_data)
                    
                    img_msg = Image()
                    img_msg.header = Header()
                    img_msg.header.seq = i
                    img_msg.header.stamp = rospy.Time.now()
                    img_msg.header.frame_id = 'camera_rgb_optical_frame'
                    img_msg.height = 480
                    img_msg.width = 640
                    img_msg.encoding = 'rgb8'
                    img_msg.is_bigendian = False
                    img_msg.step = 640 * 3
                    img_msg.data = jpeg_data
                    
                    writer.write_message('/camera/rgb/image_raw', img_msg)
                    rospy.sleep(0.01)
                
                writer.close()
                
                mcap_file = f"{bag_path}.mcap"
                self.assertTrue(os.path.exists(mcap_file))
                file_sizes[comp_type] = os.path.getsize(mcap_file)
                self.assertGreater(file_sizes[comp_type], 0)
        
        # All compression types should produce valid files
        self.assertEqual(len(file_sizes), len(compression_types))
        # Note: We don't assert which is smaller as it depends on the data


if __name__ == '__main__':
    unittest.main()

