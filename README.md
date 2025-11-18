# rosbag1_py

[![CI](https://github.com/zhexuany/rosbag1_py/actions/workflows/ci.yml/badge.svg)](https://github.com/zhexuany/rosbag1_py/actions/workflows/ci.yml)

Production-ready Python API for ROS1 bag recording with MCAP support, compression, bag splitting, service call recording, and ROS1/ROS2 interoperability.

## Features

- **C++ Core**: High-performance C++ implementation with pybind11 Python bindings
- **MCAP Support**: Native MCAP format support for ROS1/ROS2 compatibility
- **Compression**: Multiple compression formats (BZ2, LZ4, ZSTD, GZIP)
- **Bag Splitting**: Automatic bag splitting by size, duration, or message count
- **Service Recording**: Record ROS service calls
- **ROS1/ROS2 Compatibility**: Type conversion and migration tools

## Quick Start (ROS1 Noetic)

Get up and running in 5 minutes:

```bash
# 1. Source ROS1 Noetic environment
source /opt/ros/noetic/setup.bash

# 2. Install system dependencies
sudo apt-get update
sudo apt-get install -y \
    ros-noetic-desktop \
    cmake build-essential python3-dev python3-pip \
    liblz4-dev libzstd-dev libbz2-dev zlib1g-dev pkg-config

# 3. Install Python dependencies
pip3 install pybind11

# 4. Clone and install
git clone https://github.com/zhexuany/rosbag1_py.git
cd rosbag1_py
# Use --no-build-isolation to access ROS packages during build
pip3 install -e . --no-build-isolation

# 5. Verify installation
python3 -c "import rosbag1_py; print('✓ Installation successful!')"
```

**That's it!** You're ready to use rosbag1_py. See [Usage](#usage) section for examples.

> **Note for macOS users**: ROS1 Noetic is not available on macOS. Use Docker instead (see [Docker Development](#macos) section).

## Installation

### Prerequisites

**Required:**
- **ROS1 Noetic** (Ubuntu 20.04 recommended)
- **CMake >= 3.15**
- **C++17 compiler** (GCC 7+ or Clang 5+)
- **Python 3.6+** (Python 3.8 recommended)
- **pybind11** (installed via pip)
- **Compression libraries** (lz4, zstd, bz2, zlib)
- **pkg-config**

**Verify ROS1 Noetic installation:**
```bash
# Check ROS version
echo $ROS_DISTRO  # Should output: noetic

# Check ROS environment
rospack find roscpp  # Should output a path, not an error
```

### Installing System Dependencies

#### Linux (Ubuntu 20.04 / ROS1 Noetic)

**Step 1: Install ROS1 Noetic** (if not already installed)

```bash
# Setup ROS repository
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
sudo apt update

# Install ROS1 Noetic desktop (includes rospy, roscpp, etc.)
sudo apt install ros-noetic-desktop-full

# Initialize rosdep
sudo rosdep init
rosdep update

# Source ROS environment
source /opt/ros/noetic/setup.bash
echo "source /opt/ros/noetic/setup.bash" >> ~/.bashrc  # Make permanent
```

**Step 2: Install build dependencies**

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    python3-dev \
    python3-pip \
    liblz4-dev \
    libzstd-dev \
    libbz2-dev \
    zlib1g-dev \
    pkg-config \
    python3-rospkg
```

**Step 3: Install Python dependencies**

```bash
pip3 install pybind11
```

#### macOS

Since ROS1 Noetic is not natively available on macOS, we recommend using Docker (see Docker section below). However, if you have ROS1 installed via other means:

```bash
# Install dependencies via Homebrew
brew install cmake python3 lz4 zstd bzip2 zlib pkg-config

# Install pybind11
pip3 install pybind11
```

## Building from Source

### Linux (ROS1 Noetic)

**Method 1: Quick Install (Recommended)**

```bash
# Source ROS environment
source /opt/ros/noetic/setup.bash

# Clone repository
git clone https://github.com/zhexuany/rosbag1_py.git
cd rosbag1_py

# Install directly (builds and installs in one step)
# Use --no-build-isolation to access ROS packages during build
pip3 install -e . --no-build-isolation
```

**Method 2: Manual Build with CMake**

```bash
# Source ROS environment
source /opt/ros/noetic/setup.bash

# Clone repository
git clone https://github.com/zhexuany/rosbag1_py.git
cd rosbag1_py

# Build C++ extension
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Install Python package
# Use --no-build-isolation to access ROS packages during build
pip3 install -e . --no-build-isolation
```

**Method 3: Using Makefile**

```bash
source /opt/ros/noetic/setup.bash
make build
```

**Verification:**

```bash
# Source ROS environment
source /opt/ros/noetic/setup.bash

# Test import
python3 -c "import rosbag1_py; print('Installation successful!')"

# Check available classes
python3 -c "from rosbag1_py import Writer, Reader; print('API available')"
```

### macOS

Since ROS1 Noetic is not available on macOS, use Docker for development:

```bash
# Clone repository
git clone <repository-url>
cd rosbag1_py

# Build Docker image
make docker_build
# or
docker-compose build dev

# Run interactive development shell
make docker_shell
# or
docker compose run --rm dev bash

# Inside the container, build the project:
./scripts/docker_build.sh
```

For more details on Docker development, see [README_DOCKER.md](README_DOCKER.md).

### Build Options

You can customize the build with CMake options:

```bash
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3) \
    -DBUILD_TESTING=ON  # Enable tests
make -j$(nproc)
```

### Verifying Installation

After installation, verify everything works:

```bash
# 1. Source ROS environment (required!)
source /opt/ros/noetic/setup.bash

# 2. Test Python import
python3 -c "import rosbag1_py; print('✓ Import successful')"

# 3. Test API availability
python3 -c "from rosbag1_py import Writer, Reader, StorageOptions; print('✓ API available')"

# 4. Run a quick test (optional)
python3 examples/basic_recording.py  # If examples are available
```

**Common Issues:**

- **ImportError**: Make sure you've sourced ROS environment: `source /opt/ros/noetic/setup.bash`
- **Module not found**: Rebuild with `pip3 install -e .` or `make build`
- **ROS packages not found**: Verify ROS installation: `rospack find roscpp`

## Usage

### Basic Recording

The simplest way to record ROS messages:

```python
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, StorageOptions, TopicMetadata

rospy.init_node('recorder')

# Create writer and configure storage
writer = Writer()
storage_opts = StorageOptions(uri='my_bag.mcap', storage_id='mcap')
writer.open(storage_opts)

# Register topic before writing
topic_meta = TopicMetadata(
    id=0,
    name='/chatter',
    type='std_msgs/String',
    serialization_format='cdr'
)
writer.create_topic(topic_meta)

# Write messages
msg = String()
msg.data = "Hello, World!"
writer.write_message('/chatter', msg)

# Always close the writer when done
writer.close()
```

See `examples/basic_recording.py` for a complete example.

### Reading Bag Files

```python
import rospy
from rosbag1_py import Reader, StorageOptions, MessageData

rospy.init_node('player')

# Create reader and open bag
reader = Reader()
storage_opts = StorageOptions(uri='my_bag.mcap', storage_id='mcap')
reader.open(storage_opts)

# Read all messages
for message in reader.read_messages():
    print(f"Topic: {message.topic}")
    print(f"Type: {message.type}")
    print(f"Timestamp: {message.timestamp}")
    print(f"Data: {message.data}")

reader.close()
```

### Compression

Enable compression to reduce bag file size:

```python
from rosbag1_py import CompressionOptions, CompressionMode

# Available compression modes: 'none', 'bz2', 'lz4', 'zstd', 'gzip'
compression_opts = CompressionOptions(
    compression_mode=CompressionMode.LZ4,
    compression_level=4  # 1-9, higher = better compression but slower
)

writer.open(storage_opts, compression_options=compression_opts)
```

### MCAP Configuration

Configure MCAP writer options for optimal performance:

```python
# Using preset profiles
storage_opts = StorageOptions(uri='my_bag.mcap', storage_id='mcap')
storage_opts.set_mcap_options(preset_profile='fastwrite')  # or 'balanced', 'smallfile'

# Or set custom options
storage_opts.set_mcap_options(
    compression='Lz4',
    chunkSize=2048,
    compressionLevel=5
)

# Or load from YAML config file
storage_opts.load_mcap_config_file('mcap_config.yml')
```

See `examples/mcap_config_example.py` for detailed MCAP configuration examples.

### Bag Splitting

Automatically split bags by size, duration, or message count:

```python
from rosbag1_py import SplitOptions, SplitMode

split_opts = SplitOptions(
    mode=SplitMode.SIZE,
    max_size=1024 * 1024 * 1024  # 1GB per file
)

# Or split by duration (seconds)
split_opts = SplitOptions(
    mode=SplitMode.DURATION,
    max_duration=3600  # 1 hour per file
)

# Or split by message count
split_opts = SplitOptions(
    mode=SplitMode.MESSAGE_COUNT,
    max_messages=10000
)

# Splitting is handled automatically during recording
```

### Service Recording

Record ROS service calls:

```python
from rosbag1_py.services import ServiceRecorder

service_recorder = ServiceRecorder(writer)
service_recorder.register_service('/my_service', MyService)

# Create recording proxy that records all calls
proxy = service_recorder.create_recording_proxy('/my_service', MyService)

# Use proxy normally - calls are automatically recorded
response = proxy(request)
```

### Advanced: Topic Filtering

Read only specific topics:

```python
# Read only specific topics
for message in reader.read_messages(topic_filters=['/chatter', '/camera/image']):
    process_message(message)
```

### Advanced: Callback-based Reading

Use callbacks for real-time processing:

```python
def message_callback(message: MessageData):
    print(f"Received: {message.topic} at {message.timestamp}")

reader.read_messages_callback(message_callback, topic_filters=['/chatter'])
```

## Architecture

- **C++ Core**: All performance-critical operations in C++
- **Python Bindings**: pybind11 for seamless Python integration
- **Storage Backends**: MCAP and ROSBAG format support
- **Compression**: Plugin-based compression architecture

## Testing

### Running Tests

#### Linux

```bash
# Source ROS environment
source /opt/ros/noetic/setup.bash

# Build with tests enabled
mkdir -p build
cd build
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)

# Run C++ tests
ctest --output-on-failure

# Run Python tests
cd ..
pytest test/ -v

# Run with coverage
pytest test/ --cov=rosbag1_py --cov-report=html
```

#### macOS (Docker)

```bash
# Run all tests in Docker
make docker_test

# Run tests with coverage
make docker_test_coverage

# Or manually
docker-compose run --rm test-runner
```

### Test Structure

- `test/unit/` - Unit tests for individual components
- `test/integration/` - Integration tests for full workflows
- `test/compatibility/` - ROS1/ROS2 compatibility tests

## Troubleshooting

### Common Issues

**1. ImportError: No module named 'rosbag1_py'**
```bash
# Solution: Rebuild and reinstall
source /opt/ros/noetic/setup.bash
pip3 install -e . --force-reinstall --no-build-isolation
```

**2. ROS packages not found during build**
```bash
# Solution: Ensure ROS environment is sourced
source /opt/ros/noetic/setup.bash
# Verify ROS is working
rospack find roscpp
# Rebuild
pip3 install -e . --force-reinstall --no-build-isolation
```

**3. CMake errors about missing dependencies**
```bash
# Solution: Install missing system libraries
sudo apt-get install -y \
    liblz4-dev libzstd-dev libbz2-dev zlib1g-dev \
    pkg-config python3-dev
```

**4. pybind11 not found**
```bash
# Solution: Install pybind11
pip3 install pybind11
# Or install system-wide
sudo apt-get install python3-pybind11-dev  # If available
```

**5. Permission denied errors**
```bash
# Solution: Use --user flag or virtual environment
pip3 install -e . --user --no-build-isolation
# Or use virtualenv
python3 -m venv venv
source venv/bin/activate
pip install -e . --no-build-isolation
```

**6. Build fails with "catkin not found"**
```bash
# Solution: Install catkin tools
sudo apt-get install python3-catkin-tools
# Or ensure ROS desktop-full is installed
sudo apt-get install ros-noetic-desktop-full
```

**7. Tests fail to import module**
```bash
# Solution: Ensure you're in the project directory and ROS is sourced
source /opt/ros/noetic/setup.bash
cd /path/to/rosbag1_py
python3 -c "import rosbag1_py"
```

### Getting Help

- 📖 Check this README and [CONTRIBUTING.md](CONTRIBUTING.md)
- 🐛 Create a [GitHub Issue](https://github.com/zhexuany/rosbag1_py/issues/new) for bugs, questions, or feature requests
- 🔍 Search [existing issues](https://github.com/zhexuany/rosbag1_py/issues) before creating new ones
- 🔒 For security issues, create a GitHub Issue with the "security" label

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

**Quick contribution checklist:**
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Make your changes and add tests
4. Run tests: `make test` (Linux) or `make docker_test` (macOS)
5. Commit: `git commit -m "feat: add your feature"`
6. Push: `git push origin feature/your-feature`
7. Open a Pull Request

For more details, see [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Copyright 2025 Zhexuan Yang

## Acknowledgments

- MCAP library by Foxglove
- pybind11 for Python bindings
- ROS community for inspiration and tools

