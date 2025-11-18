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

