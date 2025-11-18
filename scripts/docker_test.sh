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

# Test runner script for Docker environment
# Designed for macOS development where ROS1 Noetic is not available

set -e

# Source ROS environment
source /opt/ros/noetic/setup.bash

echo "=========================================="
echo "rosbag1_py Test Suite"
echo "=========================================="
echo "ROS_DISTRO: $ROS_DISTRO"
echo "Python: $(python3 --version)"
echo "CMake: $(cmake --version | head -n1)"
echo ""

# Start ROS master in background for tests that need it
echo "Starting ROS master..."
roscore > /dev/null 2>&1 &
ROSCORE_PID=$!
sleep 2  # Give roscore time to start
echo "ROS master started (PID: $ROSCORE_PID)"

# Build the project
echo "Building rosbag1_py..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3)

# Build
echo "Compiling..."
make -j$(nproc)

# Install C++ extension
echo "Installing C++ extension..."
make install || true

cd ..

# Install Python package
echo "Installing Python package..."
pip3 install -e . --no-build-isolation || pip3 install -e .

# Verify installation
echo "Verifying installation..."
python3 -c "import rosbag1_py; print('✓ rosbag1_py imported successfully')" || {
    echo "✗ Failed to import rosbag1_py"
    echo "Checking for compiled module..."
    find . -name "*.so" -o -name "rosbag1_py_cpp*" | head -5
    exit 1
}

# Run C++ tests
echo ""
echo "=========================================="
echo "Running C++ tests..."
echo "=========================================="
cd build
if ctest --output-on-failure; then
    echo "✓ C++ tests passed"
else
    echo "✗ C++ tests failed"
    exit 1
fi
cd ..

# Run Python tests
echo ""
echo "=========================================="
echo "Running Python tests..."
echo "=========================================="
if pytest test/ -v --tb=short --timeout=300; then
    echo "✓ Python tests passed"
else
    echo "✗ Python tests failed"
    exit 1
fi

# Run with coverage if requested
if [ "$WITH_COVERAGE" = "1" ]; then
    echo ""
    echo "=========================================="
    echo "Generating coverage report..."
    echo "=========================================="
    pytest test/ -v --cov=rosbag1_py --cov-report=html --cov-report=term --cov-report=xml
    echo "Coverage report generated in htmlcov/"
    echo "Checking coverage threshold..."
    # Check if coverage is above 90%
    coverage_percent=$(pytest test/ --cov=rosbag1_py --cov-report=term-missing -q | grep "TOTAL" | awk '{print $NF}' | sed 's/%//')
    if (( $(echo "$coverage_percent < 90" | bc -l) )); then
        echo "⚠️  Warning: Coverage is ${coverage_percent}%, below 90% threshold"
    else
        echo "✅ Coverage is ${coverage_percent}%, above 90% threshold"
    fi
fi

# Cleanup: stop ROS master
if [ ! -z "$ROSCORE_PID" ]; then
    echo ""
    echo "Stopping ROS master..."
    kill $ROSCORE_PID 2>/dev/null || true
    wait $ROSCORE_PID 2>/dev/null || true
fi

echo ""
echo "=========================================="
echo "All tests completed successfully!"
echo "=========================================="

