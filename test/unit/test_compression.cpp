#include <gtest/gtest.h>
#include "rosbag1_py/compression.hpp"
#include <vector>
#include <cstring>

using namespace rosbag1_py;

class CompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test data
        test_data_.resize(1000);
        for (size_t i = 0; i < test_data_.size(); ++i) {
            test_data_[i] = static_cast<uint8_t>(i % 256);
        }
    }
    
    std::vector<uint8_t> test_data_;
};

TEST_F(CompressionTest, LZ4Compression) {
    auto compressor = CompressionFactory::create("lz4");
    ASSERT_NE(compressor, nullptr);
    
    auto compressed = compressor->compress(test_data_.data(), test_data_.size());
    EXPECT_LT(compressed.size(), test_data_.size() * 2);  // Should compress
    
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), test_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
}

TEST_F(CompressionTest, ZSTDCompression) {
    auto compressor = CompressionFactory::create("zstd");
    ASSERT_NE(compressor, nullptr);
    
    auto compressed = compressor->compress(test_data_.data(), test_data_.size());
    EXPECT_LT(compressed.size(), test_data_.size() * 2);
    
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), test_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
}

