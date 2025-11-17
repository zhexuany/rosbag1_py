cmake_minimum_required(VERSION 3.15)
project(rosbag1_py VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ROS1 packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rosbag
  std_msgs
  sensor_msgs
  geometry_msgs
)

# Find pybind11
# Try to find pybind11 via Python first (for pip installations)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
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
set_target_properties(rosbag1_py_cpp PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rosbag1_py
  PREFIX ""
  SUFFIX "${PYTHON_MODULE_EXTENSION}"
)

# Install rules
install(TARGETS rosbag1_py_cpp
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/rosbag1_py
)

# Testing (only if BUILD_TESTING is ON)
# Default to ON if not specified (for explicit test builds)
option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    
    # Only add test subdirectory if it exists (for builder stage)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
        # Pass MCAP_INCLUDE_DIR to test subdirectory as a cache variable
        set(MCAP_INCLUDE_DIR ${MCAP_INCLUDE_DIR} CACHE INTERNAL "MCAP include directory")
        add_subdirectory(test)
    endif()
endif()

