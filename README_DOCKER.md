# Docker Development Guide for macOS

Since ROS1 Noetic is not available on macOS, this project provides a Docker-based development environment with optimized multi-stage builds for better caching.

## Architecture

The Dockerfile uses a multi-stage build pattern:

1. **ros1-base**: ROS1 Noetic base with system dependencies (CACHED)
2. **rosbag1-builder**: Builds C++ extension (CACHED when source unchanged)
3. **rosbag1-dev**: Development environment with rosbag1_py installed
4. **rosbag1-test**: Lightweight test runner for CI/CD

## Quick Start

### Prerequisites
- Docker Desktop for Mac
- Docker Compose (usually included with Docker Desktop)

### Build Docker Image

```bash
make docker_build
# or
docker-compose build dev
```

### Run Tests

```bash
make docker_test
# or
docker-compose run --rm test-runner
```

### Interactive Development Shell

```bash
make docker_shell
# or
docker-compose run --rm dev bash
```

Inside the container:
```bash
# ROS environment is already sourced
# Build the project
./scripts/docker_build.sh

# Run tests
./scripts/docker_test.sh

# Or run specific tests
pytest test/unit/test_writer.py -v
```

## Available Commands

### Using Makefile (Recommended)

```bash
make docker_build          # Build Docker image
make docker_test           # Run all tests
make docker_test_coverage  # Run tests with coverage
make docker_shell          # Open interactive shell
make docker_builder        # Build C++ extension only
make docker_clean          # Clean Docker resources
```

### Using Docker Compose Directly

```bash
# Build development environment
docker-compose build dev

# Build test runner
docker-compose build test-runner

# Run tests
docker-compose run --rm test-runner

# Interactive shell
docker-compose run --rm dev bash

# Build C++ extension only
docker-compose run --rm builder

# Clean up
docker-compose down -v
```

## Development Workflow

1. **Edit code** on your Mac (files are mounted as volumes)

2. **Build and test** in Docker:
   ```bash
   make docker_test
   ```

3. **Debug interactively**:
   ```bash
   make docker_shell
   # Inside container:
   ./scripts/docker_build.sh
   python3 -c "import rosbag1_py; print('OK')"
   ```

## Volume Mounting

The project directory is mounted at `/workspace` in the container, so:
- Code changes on Mac are immediately available in container
- Build artifacts persist in `build/` directory
- Test results are available on Mac

## Layer Caching

The multi-stage build optimizes Docker layer caching:

1. **ros1-base stage**: Cached unless ROS1 or system dependencies change
2. **rosbag1-builder stage**: 
   - CMake configuration cached unless `CMakeLists.txt` changes
   - Dependencies cached unless `CMakeLists.txt` or source files change
   - Only rebuilds when C++ source code changes
3. **rosbag1-dev stage**: Rebuilds when Python code or configuration changes

This means:
- First build: ~10-15 minutes (downloads ROS1, builds everything)
- Subsequent builds: ~1-2 minutes (only rebuilds changed layers)
- Code-only changes: ~30 seconds (only rebuilds affected stage)

## ROS Environment

The Docker container includes:
- ROS1 Noetic desktop-full
- All ROS dependencies
- Python 3 with rospy
- Build tools (CMake, g++, etc.)
- Compression libraries (lz4, zstd, bz2, zlib)

ROS environment is automatically sourced in:
- Interactive shells
- Test scripts
- Build scripts

## Troubleshooting

### Container won't start
```bash
# Check Docker is running
docker ps

# Rebuild from scratch
make docker_clean
make docker_build
```

### Tests fail to import module
```bash
# Rebuild in container
make docker_shell
# Inside:
./scripts/docker_build.sh
python3 -c "import rosbag1_py"
```

### Permission issues
```bash
# Fix file permissions
sudo chown -R $(whoami) .
```

### Network issues (ROS master)
```bash
# For ROS communication, you may need to set:
export ROS_MASTER_URI=http://localhost:11311
```

## CI/CD Integration

The Docker setup is also suitable for CI/CD:

```yaml
# Example GitHub Actions
- name: Run tests
  run: |
    docker-compose build
    docker-compose run --rm test-runner
```

