#include <gtest/gtest.h>
#include "rosbag1_py/splitting.hpp"
#include "rosbag1_py/constants.hpp"
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class SplittingTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "/tmp/rosbag1_py_test_" + std::to_string(getpid());
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
    std::string get_test_path(const std::string& filename) {
        return test_dir_ + "/" + filename;
    }
};

// Test size-based splitting
TEST_F(SplittingTest, SizeBasedSplitting) {
    SplitOptions opts;
    opts.mode = SplitMode::SIZE;
    opts.max_size = 1024;  // 1KB
    
    BagSplitter splitter(opts);
    
    // Should not split initially
    EXPECT_FALSE(splitter.should_split(0.0, 100));
    
    // Update counters
    splitter.update_counters(500, 1.0);
    EXPECT_FALSE(splitter.should_split(2.0, 400));  // 500 + 400 = 900 < 1024
    
    splitter.update_counters(400, 2.0);
    EXPECT_TRUE(splitter.should_split(3.0, 200));  // 900 + 200 = 1100 > 1024
}

// Test duration-based splitting
TEST_F(SplittingTest, DurationBasedSplitting) {
    SplitOptions opts;
    opts.mode = SplitMode::DURATION;
    opts.max_duration = 10.0;  // 10 seconds
    
    BagSplitter splitter(opts);
    
    splitter.update_counters(100, 0.0);
    EXPECT_FALSE(splitter.should_split(5.0, 100));
    EXPECT_FALSE(splitter.should_split(9.0, 100));
    EXPECT_TRUE(splitter.should_split(10.0, 100));
    EXPECT_TRUE(splitter.should_split(15.0, 100));
}

// Test message count-based splitting
TEST_F(SplittingTest, MessageCountBasedSplitting) {
    SplitOptions opts;
    opts.mode = SplitMode::MESSAGE_COUNT;
    opts.max_messages = 5;
    
    BagSplitter splitter(opts);
    
    splitter.update_counters(100, 1.0);
    EXPECT_FALSE(splitter.should_split(2.0, 100));
    
    for (int i = 0; i < 4; ++i) {
        splitter.update_counters(100, static_cast<double>(i + 2));
    }
    
    EXPECT_TRUE(splitter.should_split(6.0, 100));  // 5 messages total
}

// Test filename generation
TEST_F(SplittingTest, FilenameGeneration) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_{index:03d}";
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test_bag");
    std::string filename1 = splitter.get_next_filename(base_uri, "bag");
    std::string filename2 = splitter.get_next_filename(base_uri, "bag");
    
    EXPECT_NE(filename1, filename2);
    EXPECT_TRUE(filename1.find("test_bag_000") != std::string::npos);
    EXPECT_TRUE(filename2.find("test_bag_001") != std::string::npos);
}

// Test counter reset
TEST_F(SplittingTest, CounterReset) {
    SplitOptions opts;
    opts.mode = SplitMode::MESSAGE_COUNT;
    opts.max_messages = 3;
    
    BagSplitter splitter(opts);
    
    splitter.update_counters(100, 1.0);
    splitter.update_counters(100, 2.0);
    splitter.update_counters(100, 3.0);
    
    EXPECT_TRUE(splitter.should_split(4.0, 100));
    
    splitter.reset_counters();
    
    EXPECT_FALSE(splitter.should_split(5.0, 100));
    EXPECT_EQ(splitter.get_current_index(), 3);  // Index should persist
}

// Test that mutable keyword is removed (compilation test)
TEST_F(SplittingTest, NoMutableKeyword) {
    SplitOptions opts;
    BagSplitter splitter(opts);
    
    // These should compile without mutable
    splitter.update_counters(100, 1.0);
    bool result = splitter.should_split(2.0, 50);
    (void)result;  // Suppress unused warning
    
    // get_current_index should be const
    uint32_t index = splitter.get_current_index();
    (void)index;
}

// Test should_split when start_time is 0.0 (should return false)
TEST_F(SplittingTest, ShouldSplitWhenStartTimeZero) {
    SplitOptions opts;
    opts.mode = SplitMode::SIZE;
    opts.max_size = 100;
    
    BagSplitter splitter(opts);
    
    // Before update_counters is called, start_time_ is 0.0
    EXPECT_FALSE(splitter.should_split(1.0, 50));
    EXPECT_FALSE(splitter.should_split(10.0, 200));
}

// Test default case in switch (invalid mode)
TEST_F(SplittingTest, InvalidSplitMode) {
    SplitOptions opts;
    opts.mode = static_cast<SplitMode>(999);  // Invalid mode
    
    BagSplitter splitter(opts);
    splitter.update_counters(100, 1.0);
    
    // Should return false for invalid mode
    EXPECT_FALSE(splitter.should_split(2.0, 50));
}

// Test filename generation with missing {basename}
TEST_F(SplittingTest, FilenameGenerationMissingBasename) {
    SplitOptions opts;
    opts.naming_pattern = "file_{index:03d}";  // No {basename}
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test_bag");
    std::string filename = splitter.get_next_filename(base_uri, "bag");
    
    // Should still generate filename, just without basename replacement
    EXPECT_TRUE(filename.find("file_000") != std::string::npos);
    EXPECT_TRUE(filename.find(".bag") != std::string::npos);
}

// Test filename generation with missing {index:03d}
TEST_F(SplittingTest, FilenameGenerationMissingIndex) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_split";  // No {index:03d}
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test_bag");
    std::string filename1 = splitter.get_next_filename(base_uri, "bag");
    std::string filename2 = splitter.get_next_filename(base_uri, "bag");
    
    // Should still generate filenames, but they'll be the same without index
    EXPECT_TRUE(filename1.find("test_bag_split") != std::string::npos);
    EXPECT_TRUE(filename2.find("test_bag_split") != std::string::npos);
}

// Test filename generation with MCAP extension
TEST_F(SplittingTest, FilenameGenerationMcapExtension) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_{index:03d}";
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test_bag");
    std::string filename = splitter.get_next_filename(base_uri, "mcap");
    
    EXPECT_TRUE(filename.find("test_bag_000") != std::string::npos);
    EXPECT_TRUE(filename.find(".mcap") != std::string::npos);
    EXPECT_TRUE(filename.find(".bag") == std::string::npos);
}

// Test filename generation with empty directory
TEST_F(SplittingTest, FilenameGenerationEmptyDirectory) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_{index:03d}";
    
    BagSplitter splitter(opts);
    
    // Use relative path (no directory)
    std::string filename = splitter.get_next_filename("test_bag", "bag");
    
    // Should handle empty directory gracefully
    EXPECT_TRUE(filename.find("test_bag_000") != std::string::npos);
    EXPECT_TRUE(filename.find(".bag") != std::string::npos);
}

// Test filename generation with absolute path
TEST_F(SplittingTest, FilenameGenerationAbsolutePath) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_{index:03d}";
    
    BagSplitter splitter(opts);
    
    std::string absolute_path = get_test_path("test_bag");
    std::string filename = splitter.get_next_filename(absolute_path, "bag");
    
    EXPECT_TRUE(filename.find("test_bag_000") != std::string::npos);
    EXPECT_TRUE(filename.find(".bag") != std::string::npos);
    // Should preserve directory structure
    EXPECT_TRUE(filename.find(test_dir_) != std::string::npos);
}

// Test update_counters when start_time is already set
TEST_F(SplittingTest, UpdateCountersWithExistingStartTime) {
    SplitOptions opts;
    opts.mode = SplitMode::DURATION;
    opts.max_duration = 10.0;
    
    BagSplitter splitter(opts);
    
    // Set start time
    splitter.update_counters(100, 5.0);
    
    // Update again - start_time should not change
    splitter.update_counters(200, 7.0);
    splitter.update_counters(300, 9.0);
    
    // Duration should be calculated from original start_time (5.0)
    EXPECT_FALSE(splitter.should_split(14.0, 100));  // 14.0 - 5.0 = 9.0 < 10.0
    EXPECT_TRUE(splitter.should_split(15.0, 100));    // 15.0 - 5.0 = 10.0 >= 10.0
}

// Test reset_counters preserves index but resets others
TEST_F(SplittingTest, ResetCountersPreservesIndex) {
    SplitOptions opts;
    opts.mode = SplitMode::MESSAGE_COUNT;
    opts.max_messages = 3;
    
    BagSplitter splitter(opts);
    
    // Update counters multiple times
    splitter.update_counters(100, 1.0);
    splitter.update_counters(200, 2.0);
    splitter.update_counters(300, 3.0);
    
    uint32_t index_before = splitter.get_current_index();
    EXPECT_TRUE(splitter.should_split(4.0, 100));
    
    // Get next filename to increment index
    splitter.get_next_filename("test", "bag");
    uint32_t index_after = splitter.get_current_index();
    EXPECT_GT(index_after, index_before);
    
    // Reset counters
    splitter.reset_counters();
    
    // Index should persist
    EXPECT_EQ(splitter.get_current_index(), index_after);
    
    // But counters should be reset
    EXPECT_FALSE(splitter.should_split(5.0, 100));
    
    // After reset, first update should set new start_time
    splitter.update_counters(100, 10.0);
    EXPECT_FALSE(splitter.should_split(11.0, 100));
}

// Test size-based splitting edge case - exact size match
TEST_F(SplittingTest, SizeBasedSplittingExactMatch) {
    SplitOptions opts;
    opts.mode = SplitMode::SIZE;
    opts.max_size = 1000;
    
    BagSplitter splitter(opts);
    
    splitter.update_counters(500, 1.0);
    EXPECT_FALSE(splitter.should_split(2.0, 499));  // 500 + 499 = 999 < 1000
    
    // Exact match should trigger split
    EXPECT_TRUE(splitter.should_split(3.0, 500));  // 500 + 500 = 1000 >= 1000
}

// Test duration-based splitting edge case - exact duration match
TEST_F(SplittingTest, DurationBasedSplittingExactMatch) {
    SplitOptions opts;
    opts.mode = SplitMode::DURATION;
    opts.max_duration = 10.0;
    
    BagSplitter splitter(opts);
    
    splitter.update_counters(100, 0.0);
    EXPECT_FALSE(splitter.should_split(9.9, 100));  // 9.9 - 0.0 = 9.9 < 10.0
    
    // Exact match should trigger split
    EXPECT_TRUE(splitter.should_split(10.0, 100));  // 10.0 - 0.0 = 10.0 >= 10.0
}

// Test message count-based splitting edge case - exact count match
TEST_F(SplittingTest, MessageCountBasedSplittingExactMatch) {
    SplitOptions opts;
    opts.mode = SplitMode::MESSAGE_COUNT;
    opts.max_messages = 5;
    
    BagSplitter splitter(opts);
    
    for (int i = 0; i < 4; ++i) {
        splitter.update_counters(100, static_cast<double>(i + 1));
    }
    
    EXPECT_FALSE(splitter.should_split(5.0, 100));  // 4 messages < 5
    
    // Exact match should trigger split
    splitter.update_counters(100, 5.0);
    EXPECT_TRUE(splitter.should_split(6.0, 100));  // 5 messages >= 5
}

// Test filename generation with complex pattern
TEST_F(SplittingTest, FilenameGenerationComplexPattern) {
    SplitOptions opts;
    opts.naming_pattern = "bag_{basename}_part_{index:03d}_final";
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test");
    std::string filename = splitter.get_next_filename(base_uri, "bag");
    
    EXPECT_TRUE(filename.find("bag_test_part_000_final") != std::string::npos);
    EXPECT_TRUE(filename.find(".bag") != std::string::npos);
}

// Test multiple filename generations increment index correctly
TEST_F(SplittingTest, MultipleFilenameGenerations) {
    SplitOptions opts;
    opts.naming_pattern = "{basename}_{index:03d}";
    
    BagSplitter splitter(opts);
    
    std::string base_uri = get_test_path("test");
    
    for (int i = 0; i < 5; ++i) {
        std::string filename = splitter.get_next_filename(base_uri, "bag");
        std::string expected_index = std::to_string(i);
        if (expected_index.length() == 1) {
            expected_index = "00" + expected_index;
        } else if (expected_index.length() == 2) {
            expected_index = "0" + expected_index;
        }
        EXPECT_TRUE(filename.find("test_" + expected_index) != std::string::npos);
    }
    
    EXPECT_EQ(splitter.get_current_index(), 5);
}

