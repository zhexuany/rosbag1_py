# Test CMakeLists.txt

# Get MCAP_INCLUDE_DIR from parent scope
# MCAP_INCLUDE_DIR should be set in the main CMakeLists.txt
# Try to find it from common locations
if(NOT DEFINED MCAP_INCLUDE_DIR)
    if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/../_deps/mcap-src/cpp/mcap/include)
        set(MCAP_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/../_deps/mcap-src/cpp/mcap/include)
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/build/_deps/mcap-src/cpp/mcap/include)
        set(MCAP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/build/_deps/mcap-src/cpp/mcap/include)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${catkin_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    include_directories(${MCAP_INCLUDE_DIR})
endif()

# Find compression libraries (same as main CMakeLists.txt)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LZ4 REQUIRED liblz4)
pkg_check_modules(ZSTD REQUIRED libzstd)

# C++ unit tests
add_executable(test_writer unit/test_writer.cpp
    ../src/rosbag1_py/writer.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/compression.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_writer PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_writer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
# Add MCAP include directory if available
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_writer PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_reader unit/test_reader.cpp
    ../src/rosbag1_py/reader.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
)
target_link_libraries(test_reader PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${ZSTD_LIBRARIES}
    zstd
)
target_include_directories(test_reader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${ZSTD_INCLUDE_DIRS}
)
# Add MCAP include directory if available
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_reader PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_compression unit/test_compression.cpp
    ../src/rosbag1_py/compression.cpp
)
target_link_libraries(test_compression PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_compression PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)

# Add tests
add_test(NAME test_writer COMMAND test_writer)
add_test(NAME test_reader COMMAND test_reader)
add_test(NAME test_compression COMMAND test_compression)

# Set test properties
set_tests_properties(test_writer PROPERTIES TIMEOUT 60)
set_tests_properties(test_reader PROPERTIES TIMEOUT 60)
set_tests_properties(test_compression PROPERTIES TIMEOUT 60)

