# ============================================================
# Stage 1: ROS1 Noetic base environment with dependencies (CACHED)
# This stage is only rebuilt when ROS1 or system dependencies change
# ============================================================
FROM ros:noetic-ros-core AS ros1-base

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=noetic
ENV PYTHONUNBUFFERED=1
ENV TZ=UTC

# Install build dependencies (minimal, only what's actually needed)
# Install ROS1 packages needed for building (catkin, roscpp, rosbag, message packages)
RUN apt-get update && apt-get install -y \
    python3-pip \
    python3-dev \
    cmake \
    build-essential \
    git \
    pkg-config \
    liblz4-dev \
    libzstd-dev \
    libbz2-dev \
    zlib1g-dev \
    libgtest-dev \
    python3-rosdep \
    python3-catkin-pkg \
    python3-rospkg \
    ros-noetic-catkin \
    ros-noetic-roscpp \
    ros-noetic-rosbag \
    ros-noetic-std-msgs \
    ros-noetic-sensor-msgs \
    ros-noetic-geometry-msgs \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Initialize rosdep (CACHED - only runs once)
RUN rosdep init || true

# Install Python dependencies (CACHED - only rebuilds if requirements change)
# Install pybind11 with CMake support so CMake can find it
RUN pip3 install --upgrade pip setuptools wheel && \
    pip3 install \
    pybind11[global]>=2.10.0 \
    pytest>=6.0 \
    pytest-cov \
    pytest-timeout \
    typing_extensions && \
    # Verify pybind11 CMake files are available
    python3 -c "import pybind11; print('pybind11 path:', pybind11.get_cmake_dir())"

# Set up ROS1 environment (CACHED)
RUN echo "source /opt/ros/noetic/setup.bash" >> ~/.bashrc && \
    echo "source /opt/ros/noetic/setup.bash" >> ~/.profile

# ============================================================
# Stage 2: Build rosbag1_py C++ extension (CACHED LAYER)
# This stage is only rebuilt when C++ code or CMakeLists.txt changes
# ============================================================
FROM ros1-base AS rosbag1-builder

# Set working directory
WORKDIR /build/rosbag1_py

# Copy build configuration files first (for better caching)
COPY CMakeLists.txt setup.py pyproject.toml ./
COPY include/ ./include/
COPY src/ ./src/

# Create build directory and configure CMake (CACHED - only rebuilds if CMakeLists.txt changes)
# Set pybind11_DIR so CMake can find pip-installed pybind11
RUN mkdir -p build && \
    cd build && \
    bash -c "source /opt/ros/noetic/setup.bash && \
    export pybind11_DIR=\$(python3 -c 'import pybind11; print(pybind11.get_cmake_dir())') && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DPYTHON_EXECUTABLE=\$(which python3) \
        -Dpybind11_DIR=\$pybind11_DIR && \
    echo '✅ CMake configuration cached'"

# Build dependencies and libraries (CACHED - only rebuilds if source changes)
RUN cd build && \
    bash -c "source /opt/ros/noetic/setup.bash && \
    echo 'Building rosbag1_py C++ extension...' && \
    make -j$(nproc) && \
    echo '✅ C++ extension built successfully'"

# Copy built artifacts to output directory
RUN mkdir -p /build/output && \
    find build -type f \( -name "*.so" -o -name "rosbag1_py_cpp*" \) -exec cp {} /build/output/ \; && \
    echo "✅ Build artifacts prepared:" && \
    ls -lh /build/output/

# ============================================================
# Stage 3: Development environment with rosbag1_py installed
# ============================================================
FROM ros1-base AS rosbag1-dev

# Set working directory
WORKDIR /workspace

# Copy pre-built C++ extension from builder stage
COPY --from=rosbag1-builder /build/output/* /workspace/rosbag1_py/

# Copy project files
COPY . /workspace/

# Install Python package (this will use pre-built extension)
RUN bash -c "source /opt/ros/noetic/setup.bash && \
    pip3 install -e . --no-build-isolation && \
    echo '✅ rosbag1_py installed successfully'"

# Verify installation
RUN bash -c "source /opt/ros/noetic/setup.bash && \
    python3 -c 'import rosbag1_py; print(\"✅ rosbag1_py imported successfully\")' || \
    (echo '⚠️  Warning: rosbag1_py import failed, may need to rebuild' && exit 1)"

# Create entrypoint script that sources ROS
RUN echo '#!/bin/bash\n\
set -e\n\
source /opt/ros/noetic/setup.bash\n\
exec "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]

# Default command
CMD ["/bin/bash"]

# ============================================================
# Stage 4: Test runner (lightweight, for CI/CD)
# ============================================================
FROM rosbag1-dev AS rosbag1-test

# Ensure typing_extensions is available (in case pip install -e overwrote it)
RUN pip3 install typing_extensions || true

# Copy test files
COPY test/ /workspace/test/
COPY pytest.ini /workspace/

# Set up test environment
ENV PYTEST_CURRENT_TEST=1

# Default command runs tests
CMD ["/workspace/scripts/docker_test.sh"]

