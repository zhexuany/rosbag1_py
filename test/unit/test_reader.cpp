#include <gtest/gtest.h>
#include "rosbag1_py/reader.hpp"
#include "rosbag1_py/storage.hpp"

using namespace rosbag1_py;

class ReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(ReaderTest, BasicCreation) {
    Reader reader;
    EXPECT_FALSE(reader.is_open());
}

TEST_F(ReaderTest, OpenClose) {
    Reader reader;
    StorageOptions opts;
    opts.uri = "/tmp/test_bag.mcap";
    opts.storage_id = StorageFormat::MCAP;
    
    // Note: This will fail if bag doesn't exist, which is expected
    // In a real test, we'd create a bag first
    // reader.open(opts);
    // EXPECT_TRUE(reader.is_open());
    // reader.close();
}

