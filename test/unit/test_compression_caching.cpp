#include <gtest/gtest.h>
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/compression.hpp"
#include "rosbag1_py/constants.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <map>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class CompressionCachingTest : public ::testing::Test {
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

// Test that compression instance is cached and reused
TEST_F(CompressionCachingTest, CompressionInstanceCached) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_compression_cache.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    CompressionOptions comp_opts;
    comp_opts.compression_mode = CompressionMode::LZ4;
    comp_opts.compression_level = 1;
    
    writer.open(storage_opts, ConverterOptions(), comp_opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    writer.create_topic(topic);
    
    // Write multiple messages - compressor should be cached
    const char* msg = "Test message for compression";
    for (int i = 0; i < 10; ++i) {
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            static_cast<uint64_t>(i) * NANOSECONDS_PER_SECOND);
    }
    
    writer.close();
}

// Test thread safety of CompressionFactory
TEST_F(CompressionCachingTest, CompressionFactoryThreadSafety) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> error_count(0);
    
    auto worker = [&]() {
        for (int i = 0; i < operations_per_thread; ++i) {
            try {
                auto compressor = CompressionFactory::create("lz4");
                if (compressor) {
                    std::vector<uint8_t> data(100, 0x42);
                    auto compressed = compressor->compress(data.data(), data.size());
                    auto decompressed = compressor->decompress(compressed.data(), compressed.size());
                    if (decompressed.size() == data.size()) {
                        success_count++;
                    } else {
                        error_count++;
                    }
                } else {
                    error_count++;
                }
            } catch (...) {
                error_count++;
            }
        }
    };
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(error_count.load(), 0);
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);
}

// Test multiple compression types
TEST_F(CompressionCachingTest, MultipleCompressionTypes) {
    std::vector<std::string> compression_types = {"lz4", "zstd", "bz2", "gzip"};
    
    for (const auto& comp_type : compression_types) {
        if (!CompressionFactory::is_available(comp_type)) {
            continue;  // Skip if not available
        }
        
        Writer writer;
        StorageOptions storage_opts;
        storage_opts.uri = get_test_path("test_" + comp_type + ".mcap");
        storage_opts.storage_id = StorageFormat::MCAP;
        
        CompressionOptions comp_opts;
        if (comp_type == "lz4") {
            comp_opts.compression_mode = CompressionMode::LZ4;
        } else if (comp_type == "zstd") {
            comp_opts.compression_mode = CompressionMode::ZSTD;
        } else if (comp_type == "bz2") {
            comp_opts.compression_mode = CompressionMode::BZ2;
        } else if (comp_type == "gzip") {
            comp_opts.compression_mode = CompressionMode::GZIP;
        }
        
        writer.open(storage_opts, ConverterOptions(), comp_opts);
        
        TopicMetadata topic;
        topic.id = 1;
        topic.name = "/test_topic";
        topic.type = "std_msgs/String";
        topic.serialization_format = "cdr";
        writer.create_topic(topic);
        
        const char* msg = "Test compression";
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            NANOSECONDS_PER_SECOND);
        
        writer.close();
    }
}

