# Contributing to rosbag1_py

Thank you for your interest in contributing to rosbag1_py! This document provides guidelines and instructions for contributing to the project.

## Quick Start for ROS1 Noetic

### Prerequisites

Before you begin, ensure you have:

- **ROS1 Noetic** installed and configured
- **Ubuntu 20.04** (recommended) or compatible Linux distribution
- **Python 3.6+** (Python 3.8 recommended)
- **CMake >= 3.15**
- **C++17 compiler** (GCC 7+ or Clang 5+)
- **Git**

### Installation Steps

1. **Install ROS1 Noetic** (if not already installed):
   ```bash
   sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
   sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
   sudo apt update
   sudo apt install ros-noetic-desktop-full
   ```

2. **Source ROS environment**:
   ```bash
   source /opt/ros/noetic/setup.bash
   echo "source /opt/ros/noetic/setup.bash" >> ~/.bashrc  # Optional: make permanent
   ```

3. **Install system dependencies**:
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
       python3-rospkg \
       python3-catkin-tools
   ```

4. **Install Python dependencies**:
   ```bash
   pip3 install --upgrade pip
   pip3 install pybind11 pytest pytest-cov
   ```

5. **Clone and build the repository**:
   ```bash
   git clone https://github.com/zhexuany/rosbag1_py.git
   cd rosbag1_py
   
   # Quick install (recommended)
   pip3 install -e .
   
   # Or build manually
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   cd ..
   pip3 install -e .
   ```

6. **Verify installation**:
   ```bash
   source /opt/ros/noetic/setup.bash
   python3 -c "import rosbag1_py; print('Installation successful!')"
   ```

## Development Setup

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/zhexuany/rosbag1_py.git
   cd rosbag1_py
   ```

3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/rosbag1_py.git
   ```

### Development Environment

1. **Create a development branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Install in development mode**:
   ```bash
   source /opt/ros/noetic/setup.bash
   pip3 install -e ".[dev]"
   ```

3. **Build with tests enabled**:
   ```bash
   mkdir -p build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
   make -j$(nproc)
   cd ..
   ```

## Development Workflow

### Making Changes

1. **Update your branch**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Make your changes**:
   - Follow the code style guidelines (see below)
   - Add tests for new features
   - Update documentation as needed

3. **Test your changes**:
   ```bash
   # Source ROS environment
   source /opt/ros/noetic/setup.bash
   
   # Run C++ tests
   cd build
   ctest --output-on-failure
   cd ..
   
   # Run Python tests
   pytest test/ -v
   
   # Run with coverage
   pytest test/ --cov=rosbag1_py --cov-report=html
   ```

4. **Check code quality**:
   ```bash
   # Python linting (if configured)
   flake8 rosbag1_py/ --max-line-length=100
   black --check rosbag1_py/
   
   # C++ formatting (if configured)
   clang-format --dry-run --Werror src/**/*.cpp include/**/*.hpp
   ```

### Code Style Guidelines

#### Python

- Follow **PEP 8** style guide
- Use **type hints** for function signatures
- Maximum line length: **100 characters**
- Use **docstrings** for all public functions, classes, and modules
- Format code with **black** (if configured)

Example:
```python
def write_message(self, topic: str, message: Any, timestamp: Optional[int] = None) -> None:
    """
    Write a message to the bag file.
    
    Args:
        topic: Topic name to write to
        message: ROS message object
        timestamp: Optional timestamp (nanoseconds). If None, uses current time.
    
    Raises:
        RuntimeError: If writer is not open or topic not registered
    """
    ...
```

#### C++

- Follow **Google C++ Style Guide**
- Use **C++17** features
- Maximum line length: **100 characters**
- Use **const** where appropriate
- Add **Doxygen-style comments** for public APIs

Example:
```cpp
/**
 * Write a message to the bag file.
 * 
 * @param topic Topic name
 * @param message ROS message pointer
 * @param timestamp Timestamp in nanoseconds
 * @throws std::runtime_error if writer is not open
 */
void write_message(const std::string& topic, 
                  const ros::Message* message, 
                  uint64_t timestamp);
```

### Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

Example:
```
feat(writer): add compression support for LZ4

Add LZ4 compression option to Writer class with configurable
compression levels. Supports compression levels 1-9.

Fixes #123
```

### Pull Request Process

1. **Update your branch**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Push your changes**:
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create a Pull Request** on GitHub:
   - Provide a clear title and description
   - Reference related issues (e.g., "Fixes #123")
   - Include test results and any relevant screenshots/logs
   - Ensure CI tests pass

4. **Respond to feedback**:
   - Address review comments promptly
   - Make requested changes
   - Update the PR as needed

### PR Requirements

All pull requests must:

- ✅ Pass all CI tests
- ✅ Maintain or improve code coverage
- ✅ Include tests for new features
- ✅ Update documentation for user-facing changes
- ✅ Follow code style guidelines
- ✅ Have at least one approval from maintainers

## Testing Guidelines

### Writing Tests

- **Unit tests**: Test individual functions/classes in isolation
- **Integration tests**: Test complete workflows
- **Test coverage**: Aim for >80% coverage for new code

### Test Structure

```
test/
├── unit/          # Unit tests (C++ and Python)
├── integration/   # Integration tests
└── compatibility/ # ROS1/ROS2 compatibility tests
```

### Running Tests

```bash
# Source ROS environment first
source /opt/ros/noetic/setup.bash

# All tests
make test

# C++ tests only
cd build && ctest --output-on-failure

# Python tests only
pytest test/ -v

# Specific test file
pytest test/unit/test_writer.py -v

# With coverage
pytest test/ --cov=rosbag1_py --cov-report=html
```

## Project Structure

```
rosbag1_py/
├── include/rosbag1_py/    # C++ headers
├── src/rosbag1_py/        # C++ source files
├── rosbag1_py/            # Python package
│   ├── _writer.py         # Writer Python API
│   ├── _reader.py         # Reader Python API
│   ├── services/          # Service recording utilities
│   ├── compatibility/     # ROS1/ROS2 compatibility
│   └── utils/             # Utility functions
├── test/                  # Test suite
│   ├── unit/              # Unit tests
│   ├── integration/       # Integration tests
│   └── compatibility/     # Compatibility tests
├── examples/              # Example scripts
├── tools/                 # Command-line tools
├── CMakeLists.txt         # CMake build configuration
├── setup.py               # Python package setup
└── pyproject.toml         # Python project metadata
```

## Areas for Contribution

We welcome contributions in these areas:

- 🐛 **Bug fixes**: Fix issues reported in GitHub Issues
- ⚡ **Performance**: Optimize existing code
- 🎨 **Features**: New compression formats, storage backends, etc.
- 📚 **Documentation**: Improve docs, add examples, tutorials
- 🧪 **Tests**: Increase test coverage, add edge cases
- 🔧 **Tools**: Command-line utilities, migration tools
- 🌉 **Compatibility**: ROS1/ROS2 interoperability improvements

## Reporting Issues

**Create a GitHub Issue** at https://github.com/zhexuany/rosbag1_py/issues/new for all bugs, questions, and feature requests.

When reporting bugs, please include:

- **OS and version**: e.g., Ubuntu 20.04
- **ROS distribution**: e.g., ROS1 Noetic
- **Python version**: `python3 --version`
- **Steps to reproduce**: Clear, minimal steps
- **Expected behavior**: What should happen
- **Actual behavior**: What actually happens
- **Error messages/logs**: Full error output
- **Minimal example**: Code snippet that reproduces the issue

## Code Review Process

1. All PRs require at least **one approval** from maintainers
2. **CI tests must pass** before merging
3. **Code coverage** should not decrease
4. **Documentation** should be updated for user-facing changes
5. Maintainers will review within **2-3 business days**

## Getting Help

- 📖 Check the [README.md](README.md) for usage examples
- 🐛 Create a [GitHub Issue](https://github.com/zhexuany/rosbag1_py/issues/new) for questions, bugs, or feature requests
- 🔍 Search [existing issues](https://github.com/zhexuany/rosbag1_py/issues) before creating new ones
- 🔒 For security issues, create a GitHub Issue with the "security" label

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

## Acknowledgments

Thank you for contributing to rosbag1_py! Your efforts help make ROS1 bag recording better for everyone.

