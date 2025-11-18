# Test CMakeLists.txt

# Create namespace aliases for googletest targets if they don't exist
# This allows tests to use GTest::gtest regardless of how googletest was included
if(TARGET gtest AND NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
endif()
if(TARGET gtest_main AND NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()
if(TARGET gmock AND NOT TARGET GTest::gmock)
    add_library(GTest::gmock ALIAS gmock)
endif()
if(TARGET gmock_main AND NOT TARGET GTest::gmock_main)
    add_library(GTest::gmock_main ALIAS gmock_main)
endif()

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

# Additional C++ unit tests
add_executable(test_storage_cpp unit/test_storage_cpp.cpp
    ../src/rosbag1_py/storage.cpp
)
target_link_libraries(test_storage_cpp PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_storage_cpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_storage_cpp PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_compression_caching unit/test_compression_caching.cpp
    ../src/rosbag1_py/writer.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/compression.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_compression_caching PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
    pthread
)
target_include_directories(test_compression_caching PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_compression_caching PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_topic_filtering unit/test_topic_filtering.cpp
    ../src/rosbag1_py/reader.cpp
    ../src/rosbag1_py/writer.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/compression.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_topic_filtering PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_topic_filtering PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_topic_filtering PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_splitting unit/test_splitting.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_splitting PRIVATE
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(test_splitting PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(test_constants unit/test_constants.cpp
    ../src/rosbag1_py/converter.cpp
)
target_link_libraries(test_constants PRIVATE
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(test_constants PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Additional comprehensive tests
add_executable(test_converter unit/test_converter.cpp
    ../src/rosbag1_py/converter.cpp
)
target_link_libraries(test_converter PRIVATE
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(test_converter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(test_serialization unit/test_serialization.cpp
    ../src/rosbag1_py/serialization.cpp
)
target_link_libraries(test_serialization PRIVATE
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(test_serialization PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(test_reader_comprehensive unit/test_reader_comprehensive.cpp
    ../src/rosbag1_py/reader.cpp
    ../src/rosbag1_py/writer.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/compression.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_reader_comprehensive PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_reader_comprehensive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_reader_comprehensive PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_writer_comprehensive unit/test_writer_comprehensive.cpp
    ../src/rosbag1_py/writer.cpp
    ../src/rosbag1_py/storage.cpp
    ../src/rosbag1_py/compression.cpp
    ../src/rosbag1_py/serialization.cpp
    ../src/rosbag1_py/converter.cpp
    ../src/rosbag1_py/splitting.cpp
)
target_link_libraries(test_writer_comprehensive PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_writer_comprehensive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_writer_comprehensive PRIVATE ${MCAP_INCLUDE_DIR})
endif()

add_executable(test_compression_complete unit/test_compression_complete.cpp
    ../src/rosbag1_py/compression.cpp
)
target_link_libraries(test_compression_complete PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_compression_complete PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)

add_executable(test_storage_edge_cases unit/test_storage_edge_cases.cpp
    ../src/rosbag1_py/storage.cpp
)
target_link_libraries(test_storage_edge_cases PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${catkin_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZSTD_LIBRARIES}
    bz2
    z
)
target_include_directories(test_storage_edge_cases PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LZ4_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
)
if(MCAP_INCLUDE_DIR)
    target_include_directories(test_storage_edge_cases PRIVATE ${MCAP_INCLUDE_DIR})
endif()

# Add tests
add_test(NAME test_writer COMMAND test_writer)
add_test(NAME test_reader COMMAND test_reader)
add_test(NAME test_compression COMMAND test_compression)
add_test(NAME test_storage_cpp COMMAND test_storage_cpp)
add_test(NAME test_compression_caching COMMAND test_compression_caching)
add_test(NAME test_topic_filtering COMMAND test_topic_filtering)
add_test(NAME test_splitting COMMAND test_splitting)
add_test(NAME test_constants COMMAND test_constants)
add_test(NAME test_converter COMMAND test_converter)
add_test(NAME test_serialization COMMAND test_serialization)
add_test(NAME test_reader_comprehensive COMMAND test_reader_comprehensive)
add_test(NAME test_writer_comprehensive COMMAND test_writer_comprehensive)
add_test(NAME test_compression_complete COMMAND test_compression_complete)
add_test(NAME test_storage_edge_cases COMMAND test_storage_edge_cases)

# Set test properties
set_tests_properties(test_writer PROPERTIES TIMEOUT 60)
set_tests_properties(test_reader PROPERTIES TIMEOUT 60)
set_tests_properties(test_compression PROPERTIES TIMEOUT 60)
set_tests_properties(test_storage_cpp PROPERTIES TIMEOUT 60)
set_tests_properties(test_compression_caching PROPERTIES TIMEOUT 60)
set_tests_properties(test_topic_filtering PROPERTIES TIMEOUT 60)
set_tests_properties(test_splitting PROPERTIES TIMEOUT 60)
set_tests_properties(test_constants PROPERTIES TIMEOUT 60)
set_tests_properties(test_converter PROPERTIES TIMEOUT 60)
set_tests_properties(test_serialization PROPERTIES TIMEOUT 60)
set_tests_properties(test_reader_comprehensive PROPERTIES TIMEOUT 120)
set_tests_properties(test_writer_comprehensive PROPERTIES TIMEOUT 120)
set_tests_properties(test_compression_complete PROPERTIES TIMEOUT 60)
set_tests_properties(test_storage_edge_cases PROPERTIES TIMEOUT 60)

