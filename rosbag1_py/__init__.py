"""
rosbag1_py - Production-ready Python API for ROS1 bag recording with MCAP support.
"""

__version__ = "1.0.0"

# Import C++ extension module
try:
    from . import rosbag1_py_cpp
except ImportError:
    raise ImportError(
        "rosbag1_py_cpp module not found. "
        "Please build the package using: python setup.py build_ext --inplace"
    )

# Import Python wrappers
from ._writer import Writer, StorageOptions, ConverterOptions, CompressionOptions, CompressionMode, StorageFormat, TopicMetadata, SplitOptions
from ._reader import Reader, MessageData

__all__ = [
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

