#include <gtest/gtest.h>
#include "rosbag1_py/compression.hpp"
#include <vector>
#include <cstring>

using namespace rosbag1_py;

class CompressionCompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data with patterns for better compression
        test_data_.resize(10000);
        for (size_t i = 0; i < test_data_.size(); ++i) {
            test_data_[i] = static_cast<uint8_t>((i / 100) % 256);
        }
        
        // Create random-like data
        random_data_.resize(5000);
        for (size_t i = 0; i < random_data_.size(); ++i) {
            random_data_[i] = static_cast<uint8_t>((i * 17 + 23) % 256);
        }
    }
    
    std::vector<uint8_t> test_data_;
    std::vector<uint8_t> random_data_;
};

// Test BZ2 compression
TEST_F(CompressionCompleteTest, BZ2Compression) {
    if (!CompressionFactory::is_available("bz2")) {
        GTEST_SKIP() << "BZ2 compression not available";
    }
    
    auto compressor = CompressionFactory::create("bz2");
    ASSERT_NE(compressor, nullptr);
    EXPECT_EQ(compressor->get_name(), "bz2");
    
    // Test compression
    auto compressed = compressor->compress(test_data_.data(), test_data_.size(), 9);
    EXPECT_LT(compressed.size(), test_data_.size() * 2);
    
    // Test decompression
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), test_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
}

// Test BZ2 with different compression levels
TEST_F(CompressionCompleteTest, BZ2CompressionLevels) {
    if (!CompressionFactory::is_available("bz2")) {
        GTEST_SKIP() << "BZ2 compression not available";
    }
    
    auto compressor = CompressionFactory::create("bz2");
    
    for (int level = 1; level <= 9; ++level) {
        auto compressed = compressor->compress(test_data_.data(), test_data_.size(), level);
        EXPECT_GT(compressed.size(), 0);
        
        auto decompressed = compressor->decompress(compressed.data(), compressed.size());
        EXPECT_EQ(decompressed.size(), test_data_.size());
        EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
    }
}

// Test GZIP compression
TEST_F(CompressionCompleteTest, GZIPCompression) {
    if (!CompressionFactory::is_available("gzip")) {
        GTEST_SKIP() << "GZIP compression not available";
    }
    
    auto compressor = CompressionFactory::create("gzip");
    ASSERT_NE(compressor, nullptr);
    EXPECT_EQ(compressor->get_name(), "gzip");
    
    // Test compression
    auto compressed = compressor->compress(test_data_.data(), test_data_.size(), 6);
    EXPECT_LT(compressed.size(), test_data_.size() * 2);
    
    // Test decompression
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), test_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
}

// Test GZIP with different compression levels
TEST_F(CompressionCompleteTest, GZIPCompressionLevels) {
    if (!CompressionFactory::is_available("gzip")) {
        GTEST_SKIP() << "GZIP compression not available";
    }
    
    auto compressor = CompressionFactory::create("gzip");
    
    for (int level = 1; level <= 9; ++level) {
        auto compressed = compressor->compress(test_data_.data(), test_data_.size(), level);
        EXPECT_GT(compressed.size(), 0);
        
        auto decompressed = compressor->decompress(compressed.data(), compressed.size());
        EXPECT_EQ(decompressed.size(), test_data_.size());
        EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
    }
}

// Test LZ4 with different data
TEST_F(CompressionCompleteTest, LZ4WithRandomData) {
    auto compressor = CompressionFactory::create("lz4");
    
    auto compressed = compressor->compress(random_data_.data(), random_data_.size());
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(decompressed.size(), random_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), random_data_.data(), random_data_.size()), 0);
}

// Test ZSTD with different data
TEST_F(CompressionCompleteTest, ZSTDWithRandomData) {
    auto compressor = CompressionFactory::create("zstd");
    
    auto compressed = compressor->compress(random_data_.data(), random_data_.size(), 3);
    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(decompressed.size(), random_data_.size());
    EXPECT_EQ(memcmp(decompressed.data(), random_data_.data(), random_data_.size()), 0);
}

// Test compression with empty data
TEST_F(CompressionCompleteTest, EmptyData) {
    std::vector<uint8_t> empty;
    
    auto lz4 = CompressionFactory::create("lz4");
    auto compressed = lz4->compress(empty.data(), 0);
    EXPECT_GT(compressed.size(), 0);  // Should produce some output
    
    auto decompressed = lz4->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), 0);
}

// Test compression with single byte
TEST_F(CompressionCompleteTest, SingleByte) {
    uint8_t single_byte = 0x42;
    
    auto lz4 = CompressionFactory::create("lz4");
    auto compressed = lz4->compress(&single_byte, 1);
    auto decompressed = lz4->decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(decompressed.size(), 1);
    EXPECT_EQ(decompressed[0], single_byte);
}

// Test compression with very large data
TEST_F(CompressionCompleteTest, VeryLargeData) {
    std::vector<uint8_t> large_data(1000000);  // 1MB
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    auto zstd = CompressionFactory::create("zstd");
    auto compressed = zstd->compress(large_data.data(), large_data.size());
    EXPECT_LT(compressed.size(), large_data.size());
    
    auto decompressed = zstd->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed.size(), large_data.size());
    EXPECT_EQ(memcmp(decompressed.data(), large_data.data(), large_data.size()), 0);
}

// Test compression factory error handling
TEST_F(CompressionCompleteTest, FactoryErrorHandling) {
    EXPECT_THROW(CompressionFactory::create("nonexistent"), std::runtime_error);
    EXPECT_FALSE(CompressionFactory::is_available("nonexistent"));
}

// Test compression factory registration
TEST_F(CompressionCompleteTest, FactoryRegistration) {
    // Test that we can check availability
    EXPECT_TRUE(CompressionFactory::is_available("lz4"));
    EXPECT_TRUE(CompressionFactory::is_available("zstd"));
    
    // BZ2 and GZIP might not always be available
    if (CompressionFactory::is_available("bz2")) {
        EXPECT_NE(CompressionFactory::create("bz2"), nullptr);
    }
    
    if (CompressionFactory::is_available("gzip")) {
        EXPECT_NE(CompressionFactory::create("gzip"), nullptr);
    }
}

// Test compression level validation
TEST_F(CompressionCompleteTest, CompressionLevelValidation) {
    auto bz2 = CompressionFactory::create("bz2");
    if (bz2) {
        // Test invalid levels (should clamp to valid range)
        auto compressed1 = bz2->compress(test_data_.data(), test_data_.size(), 0);  // Too low
        auto compressed2 = bz2->compress(test_data_.data(), test_data_.size(), 10);  // Too high
        
        EXPECT_GT(compressed1.size(), 0);
        EXPECT_GT(compressed2.size(), 0);
    }
    
    auto gzip = CompressionFactory::create("gzip");
    if (gzip) {
        auto compressed1 = gzip->compress(test_data_.data(), test_data_.size(), 0);  // Too low
        auto compressed2 = gzip->compress(test_data_.data(), test_data_.size(), 10);  // Too high
        
        EXPECT_GT(compressed1.size(), 0);
        EXPECT_GT(compressed2.size(), 0);
    }
}

// Test round-trip compression for all algorithms
TEST_F(CompressionCompleteTest, RoundTripAllAlgorithms) {
    std::vector<std::string> algorithms = {"lz4", "zstd", "bz2", "gzip"};
    
    for (const auto& algo : algorithms) {
        if (!CompressionFactory::is_available(algo)) {
            continue;
        }
        
        auto compressor = CompressionFactory::create(algo);
        ASSERT_NE(compressor, nullptr);
        
        auto compressed = compressor->compress(test_data_.data(), test_data_.size());
        auto decompressed = compressor->decompress(compressed.data(), compressed.size());
        
        EXPECT_EQ(decompressed.size(), test_data_.size());
        EXPECT_EQ(memcmp(decompressed.data(), test_data_.data(), test_data_.size()), 0);
    }
}

