#!/bin/bash
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

