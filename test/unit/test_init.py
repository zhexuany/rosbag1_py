"""
Unit tests for rosbag1_py package initialization.
"""

import unittest


class TestInit(unittest.TestCase):
    """Tests for package initialization"""
    
    def test_package_import(self):
        """Test that package imports correctly"""
        import rosbag1_py
        
        # Verify version is set
        self.assertTrue(hasattr(rosbag1_py, '__version__'))
        self.assertIsNotNone(rosbag1_py.__version__)
    
    def test_all_exports(self):
        """Test that all expected exports are available"""
        import rosbag1_py
        
        # Check all expected exports from __all__
        expected_exports = [
            'Writer',
            'Reader',
            'StorageOptions',
            'ConverterOptions',
            'CompressionOptions',
            'CompressionMode',
            'StorageFormat',
            'TopicMetadata',
            'MessageData',
            'SplitOptions',
        ]
        
        for export in expected_exports:
            self.assertTrue(hasattr(rosbag1_py, export), 
                          f"Missing export: {export}")
    
    def test_classes_importable(self):
        """Test that main classes can be instantiated"""
        from rosbag1_py import Reader, Writer
        
        # Should be able to create instances
        reader = Reader()
        writer = Writer()
        
        self.assertIsNotNone(reader)
        self.assertIsNotNone(writer)
    
    def test_cpp_module_loaded(self):
        """Test that C++ extension module is loaded"""
        import rosbag1_py
        
        # C++ module should be accessible
        self.assertTrue(hasattr(rosbag1_py, 'rosbag1_py_cpp'))


if __name__ == '__main__':
    unittest.main()

