# rosbag1_py Tests

This directory contains comprehensive tests for rosbag1_py.

## Test Structure

### Unit Tests

**C++ Unit Tests** (using Google Test):
- `unit/test_writer.cpp` - Writer class tests
- `unit/test_reader.cpp` - Reader class tests
- `unit/test_compression.cpp` - Compression algorithm tests

**Python Unit Tests** (using unittest):
- `unit/test_writer.py` - Python Writer wrapper tests
- `unit/test_reader.py` - Python Reader wrapper tests
- `unit/test_compression.py` - Compression configuration tests
- `unit/test_storage.py` - Storage backend tests

### Integration Tests

- `integration/test_recording_playback.py` - End-to-end recording/playback
- `integration/test_compression_integration.py` - Compression with real data
- `integration/test_splitting.py` - Bag splitting functionality
- `integration/test_service_recording.py` - Service call recording
- `integration/test_metadata.py` - Bag metadata operations

### Compatibility Tests

- `compatibility/test_type_conversion.py` - ROS1/ROS2 type conversion

## Running Tests

### C++ Tests

```bash
cd build
cmake ..
make
ctest --output-on-failure
```

Or run individual tests:
```bash
./test_writer
./test_reader
./test_compression
```

### Python Tests

```bash
# Run all tests
pytest test/ -v

# Run specific test categories
pytest test/unit/ -v
pytest test/integration/ -v
pytest test/compatibility/ -v

# Run with coverage
pytest test/ --cov=rosbag1_py --cov-report=html
```

### Docker Tests

```bash
docker build -t rosbag1_py:noetic .
docker run -it rosbag1_py:noetic bash
# Inside container:
./scripts/docker_test.sh
```

## Test Coverage Goals

- Unit Tests: >90% coverage
- Integration Tests: All major workflows
- Compatibility Tests: All conversion paths

