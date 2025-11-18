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

cmake_minimum_required(VERSION 3.15)
project(rosbag1_py VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Workaround for old system googletest compatibility issue
# Set policy version minimum to suppress warnings from old googletest CMakeLists.txt
# This is needed because system-installed googletest may have outdated CMake requirements
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()
# Set minimum policy version to handle old googletest if it's found
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)

# Add custom CMake modules directory to module path
# This provides FindPythonInterp.cmake shim for catkin compatibility with CMake 3.27+
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find Python FIRST - before catkin
# The FindPythonInterp.cmake shim in cmake/ will handle the legacy module compatibility
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 found: ${Python3_EXECUTABLE} (version ${Python3_VERSION})")

# Find ROS1 packages (now catkin will see the Python variables it expects)
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rosbag
  std_msgs
  sensor_msgs
  geometry_msgs
)

# Find pybind11
# Try to find pybind11 via Python first (for pip installations)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(pybind11_DIR)
    set(pybind11_DIR ${pybind11_DIR} CACHE PATH "pybind11 CMake directory")
endif()
find_package(pybind11 REQUIRED)

# Find compression libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(LZ4 REQUIRED liblz4)
pkg_check_modules(ZSTD REQUIRED libzstd)

# Fetch MCAP C++ library (header-only)
include(FetchContent)
FetchContent_Declare(
    mcap
    GIT_REPOSITORY https://github.com/foxglove/mcap.git
    GIT_TAG releases/cpp/v2.1.1
    GIT_SHALLOW TRUE
)
# Only fetch if not already available
if(NOT mcap_POPULATED)
    FetchContent_MakeAvailable(mcap)
endif()

# MCAP is a header-only library
# The headers are in cpp/mcap/include/mcap/
# We just need to add the include directory, no linking needed
# FetchContent sets mcap_SOURCE_DIR after MakeAvailable
if(DEFINED mcap_SOURCE_DIR AND EXISTS ${mcap_SOURCE_DIR}/cpp/mcap/include)
    set(MCAP_INCLUDE_DIR ${mcap_SOURCE_DIR}/cpp/mcap/include)
    message(STATUS "MCAP include directory: ${MCAP_INCLUDE_DIR}")
elseif(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/_deps/mcap-src/cpp/mcap/include)
    set(MCAP_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/mcap-src/cpp/mcap/include)
    message(STATUS "MCAP include directory (fallback): ${MCAP_INCLUDE_DIR}")
else()
    message(WARNING "MCAP include directory not found - MCAP features will not be available")
    set(MCAP_INCLUDE_DIR "")
endif()

# Include directories
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
  src/rosbag1_py/writer.cpp
  src/rosbag1_py/reader.cpp
  src/rosbag1_py/storage.cpp
  src/rosbag1_py/serialization.cpp
  src/rosbag1_py/compression.cpp
  src/rosbag1_py/splitting.cpp
  src/rosbag1_py/converter.cpp
  src/rosbag1_py/bindings.cpp
)

# Header files
set(HEADERS
  include/rosbag1_py/writer.hpp
  include/rosbag1_py/reader.hpp
  include/rosbag1_py/storage.hpp
  include/rosbag1_py/compression.hpp
  include/rosbag1_py/converter.hpp
  include/rosbag1_py/serialization.hpp
  include/rosbag1_py/splitting.hpp
)

# Create pybind11 module
pybind11_add_module(rosbag1_py_cpp ${SOURCES})

# Link libraries
target_link_libraries(rosbag1_py_cpp PRIVATE
  ${catkin_LIBRARIES}
  ${LZ4_LIBRARIES}
  ${ZSTD_LIBRARIES}
  bz2
  z
  pybind11::module
)

# MCAP is header-only, so we only need include directories
if(MCAP_INCLUDE_DIR)
    target_include_directories(rosbag1_py_cpp PRIVATE ${MCAP_INCLUDE_DIR})
    message(STATUS "MCAP header-only library included")
else()
    message(WARNING "MCAP include directory not found - MCAP features will not be available")
endif()

# Include directories for target
target_include_directories(rosbag1_py_cpp PRIVATE
  ${catkin_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LZ4_INCLUDE_DIRS}
  ${ZSTD_INCLUDE_DIRS}
)
# MCAP includes are handled above (header-only library)

# Compiler flags
target_compile_options(rosbag1_py_cpp PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# Set output directory
# If CMAKE_LIBRARY_OUTPUT_DIRECTORY is set (by setup.py), use it directly
# setup.py already includes the rosbag1_py subdirectory in the path
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY OR CMAKE_LIBRARY_OUTPUT_DIRECTORY STREQUAL "")
  # Manual build: output to source directory
  set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rosbag1_py)
else()
  # pip install: use the path provided by setup.py (already includes package dir)
  set(OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
endif()

set_target_properties(rosbag1_py_cpp PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
  PREFIX ""
  SUFFIX "${PYTHON_MODULE_EXTENSION}"
)

# Install rules
install(TARGETS rosbag1_py_cpp
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/rosbag1_py
)

# Testing (only if BUILD_TESTING is ON)
# Default to OFF for pip installs (tests are built separately)
# Can be set to ON explicitly for test builds
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    
    # Use FetchContent for googletest if targets don't already exist
    # System googletest (from libgtest-dev) may auto-create targets, so check first
    if(NOT TARGET gtest AND NOT TARGET gtest_main)
        # Disable finding system googletest to avoid conflicts
        set(CMAKE_DISABLE_FIND_PACKAGE_GTest TRUE)
        
        # Use FetchContent to get googletest
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
            GIT_SHALLOW TRUE
        )
        if(NOT googletest_POPULATED)
            set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
        endif()
    else()
        message(STATUS "Using existing googletest targets")
    endif()
    
    # Only add test subdirectory if it exists (for builder stage)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
        # Pass MCAP_INCLUDE_DIR to test subdirectory as a cache variable
        set(MCAP_INCLUDE_DIR ${MCAP_INCLUDE_DIR} CACHE INTERNAL "MCAP include directory")
        add_subdirectory(test)
    endif()
endif()

