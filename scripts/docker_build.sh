#!/bin/bash
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

# Build script for Docker environment
# Quick build without running tests

set -e

source /opt/ros/noetic/setup.bash

echo "Building rosbag1_py..."
mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3)

make -j$(nproc)
cd ..

pip3 install -e . --no-build-isolation || pip3 install -e .

echo "Build complete!"

