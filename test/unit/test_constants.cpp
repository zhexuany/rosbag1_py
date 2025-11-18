#include <gtest/gtest.h>
#include "rosbag1_py/constants.hpp"
#include "rosbag1_py/converter.hpp"
#include "rosbag1_py/splitting.hpp"
#include <cstdint>

using namespace rosbag1_py;

class ConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test time conversion constants
TEST_F(ConstantsTest, TimeConversionConstants) {
    // Test nanoseconds per second
    EXPECT_EQ(NANOSECONDS_PER_SECOND, 1000000000ULL);
    
    // Test seconds per nanosecond
    EXPECT_DOUBLE_EQ(SECONDS_PER_NANOSECOND, 1e-9);
    
    // Test conversion using constants
    uint64_t nanoseconds = 5 * NANOSECONDS_PER_SECOND;
    EXPECT_EQ(nanoseconds, 5000000000ULL);
    
    double seconds = nanoseconds * SECONDS_PER_NANOSECOND;
    EXPECT_DOUBLE_EQ(seconds, 5.0);
}

// Test file size constants
TEST_F(ConstantsTest, FileSizeConstants) {
    EXPECT_EQ(BYTES_PER_KB, 1024ULL);
    EXPECT_EQ(BYTES_PER_MB, 1024ULL * 1024ULL);
    EXPECT_EQ(BYTES_PER_GB, 1024ULL * 1024ULL * 1024ULL);
    
    // Test relationships
    EXPECT_EQ(BYTES_PER_MB, BYTES_PER_KB * 1024ULL);
    EXPECT_EQ(BYTES_PER_GB, BYTES_PER_MB * 1024ULL);
}

// Test default split values
TEST_F(ConstantsTest, DefaultSplitValues) {
    EXPECT_EQ(DEFAULT_MAX_SIZE_BYTES, BYTES_PER_GB);
    EXPECT_DOUBLE_EQ(DEFAULT_MAX_DURATION_SECONDS, 300.0);
    EXPECT_EQ(DEFAULT_MAX_MESSAGES, 100000ULL);
    
    // Test that SplitOptions uses these defaults
    SplitOptions opts;
    EXPECT_EQ(opts.max_size, DEFAULT_MAX_SIZE_BYTES);
    EXPECT_DOUBLE_EQ(opts.max_duration, DEFAULT_MAX_DURATION_SECONDS);
    EXPECT_EQ(opts.max_messages, DEFAULT_MAX_MESSAGES);
}

// Test TimeConverter uses constants
TEST_F(ConstantsTest, TimeConverterUsesConstants) {
    uint32_t sec = 5;
    uint32_t nsec = 500000000;
    
    uint64_t nanoseconds = TimeConverter::ros1_to_nanoseconds(sec, nsec);
    EXPECT_EQ(nanoseconds, sec * NANOSECONDS_PER_SECOND + nsec);
    
    auto [result_sec, result_nsec] = TimeConverter::nanoseconds_to_ros1(nanoseconds);
    EXPECT_EQ(result_sec, sec);
    EXPECT_EQ(result_nsec, nsec);
}

// Test that constants are compile-time evaluable
TEST_F(ConstantsTest, CompileTimeConstants) {
    // These should be evaluable at compile time
    static_assert(NANOSECONDS_PER_SECOND == 1000000000ULL);
    static_assert(BYTES_PER_KB == 1024ULL);
    static_assert(BYTES_PER_MB == 1048576ULL);
    static_assert(BYTES_PER_GB == 1073741824ULL);
}

