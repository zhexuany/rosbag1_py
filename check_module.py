#!/usr/bin/env python3
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

import sys
import os
import inspect
sys.path.insert(0, '.')

# Check file
reader_file = 'rosbag1_py/_reader.py'
mtime = os.path.getmtime(reader_file)
print(f'[CHECK] _reader.py mtime: {mtime}', file=sys.stderr)

# Read lines around the function
with open(reader_file) as f:
    lines = f.readlines()
    print(f'[CHECK] Total lines: {len(lines)}', file=sys.stderr)
    for i in range(79, min(90, len(lines))):
        print(f'[CHECK] Line {i+1}: {lines[i].rstrip()}', file=sys.stderr)

# Import and check
import rospy
rospy.init_node('test', anonymous=True)
from rosbag1_py import Reader

reader = Reader()
print(f'[CHECK] Reader object: {reader}', file=sys.stderr)

# Get source
try:
    source = inspect.getsource(reader.read_messages)
    if 'FUNCTION CALLED' in source:
        print('[CHECK] ✓ Found FUNCTION CALLED in source', file=sys.stderr)
    else:
        print('[CHECK] ✗ FUNCTION CALLED NOT in source!', file=sys.stderr)
        # Find where it starts
        if 'def read_messages' in source:
            idx = source.find('def read_messages')
            print(f'[CHECK] Function starts at char {idx}', file=sys.stderr)
            print(f'[CHECK] First 300 chars after def: {source[idx:idx+300]}', file=sys.stderr)
except Exception as e:
    print(f'[CHECK] Exception getting source: {e}', file=sys.stderr)
    import traceback
    traceback.print_exc(file=sys.stderr)

