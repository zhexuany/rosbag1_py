# rosbag1_py

Production-ready Python API for ROS1 bag recording with MCAP support, compression, bag splitting, service call recording, and ROS1/ROS2 interoperability.

## Features

- **C++ Core**: High-performance C++ implementation with pybind11 Python bindings
- **MCAP Support**: Native MCAP format support for ROS1/ROS2 compatibility
- **Compression**: Multiple compression formats (BZ2, LZ4, ZSTD, GZIP)
- **Bag Splitting**: Automatic bag splitting by size, duration, or message count
- **Service Recording**: Record ROS service calls
- **ROS1/ROS2 Compatibility**: Type conversion and migration tools

## Installation

### Prerequisites

- ROS1 Noetic (or compatible ROS1 distribution)
- CMake >= 3.15
- C++17 compiler
- Python 3.6+
- pybind11
- Compression libraries (lz4, zstd, bz2, zlib)

### Build from Source

```bash
# Source ROS environment
source /opt/ros/noetic/setup.bash

# Clone repository
git clone <repository-url>
cd rosbag1_py

# Build and install
pip install .
```

### Docker

```bash
# Build Docker image
docker build -t rosbag1_py:noetic .

# Run tests
docker run -it rosbag1_py:noetic bash
# Inside container:
source /opt/ros/noetic/setup.bash
./scripts/docker_test.sh
```

## Usage

### Basic Recording

```python
import rospy
from std_msgs.msg import String
from rosbag1_py import Writer, StorageOptions, TopicMetadata

rospy.init_node('recorder')

writer = Writer()
storage_opts = StorageOptions(uri='my_bag.mcap', storage_id='mcap')
writer.open(storage_opts)

# Register topic
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

writer.close()
```

### Compression

```python
from rosbag1_py import CompressionOptions

compression_opts = CompressionOptions(
    compression_mode='lz4',
    compression_level=4
)
writer.open(storage_opts, compression_options=compression_opts)
```

### Bag Splitting

```python
from rosbag1_py import SplitOptions, SplitMode

split_opts = SplitOptions(
    mode=SplitMode.SIZE,
    max_size=1024 * 1024 * 1024  # 1GB
)
# Splitting is handled automatically during recording
```

### Service Recording

```python
from rosbag1_py.services import ServiceRecorder

service_recorder = ServiceRecorder(writer)
service_recorder.register_service('/my_service', MyService)

# Create recording proxy
proxy = service_recorder.create_recording_proxy('/my_service', MyService)
response = proxy(request)
```

## Architecture

- **C++ Core**: All performance-critical operations in C++
- **Python Bindings**: pybind11 for seamless Python integration
- **Storage Backends**: MCAP and ROSBAG format support
- **Compression**: Plugin-based compression architecture

## Testing

```bash
# Run Python tests
pytest test/ -v

# Run C++ tests
cd build
ctest --output-on-failure
```

## Documentation

See `docs/` directory for detailed documentation.

## License

[Specify license]

## Contributing

[Contributing guidelines]

