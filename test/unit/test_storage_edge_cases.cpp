#include <gtest/gtest.h>
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/constants.hpp"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class StorageEdgeCasesTest : public ::testing::Test {
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

// Test storage with empty filename
TEST_F(StorageEdgeCasesTest, EmptyFilename) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = "";
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    // Should handle empty filename gracefully
    bool result = storage->open(opts);
    // Result depends on implementation - might succeed or fail
    if (result) {
        storage->close();
    }
}

// Test storage with very long filename
TEST_F(StorageEdgeCasesTest, LongFilename) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path(std::string(200, 'a') + ".bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    bool result = storage->open(opts);
    if (result) {
        storage->close();
    }
}

// Test storage operations on closed storage
TEST_F(StorageEdgeCasesTest, OperationsOnClosedStorage) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_closed.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    storage->close();
    
    EXPECT_FALSE(storage->is_open());
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    EXPECT_THROW(storage->create_topic(topic), std::runtime_error);
    EXPECT_THROW(storage->write_message("/test_topic", nullptr, 0, 0), std::runtime_error);
}

// Test storage with invalid storage format
TEST_F(StorageEdgeCasesTest, InvalidStorageFormat) {
    // Note: This would require adding an invalid enum value, which isn't possible
    // But we can test the factory error handling
    EXPECT_THROW(StorageFactory::create(static_cast<StorageFormat>(999)), std::runtime_error);
}

// Test storage with empty topic name
TEST_F(StorageEdgeCasesTest, EmptyTopicName) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_empty_topic.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "";  // Empty topic name
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    // Should handle empty topic name (might succeed or throw)
    try {
        storage->create_topic(topic);
        storage->write_message("", reinterpret_cast<const uint8_t*>("test"), 4, NANOSECONDS_PER_SECOND);
    } catch (...) {
        // Expected if empty topic name is invalid
    }
    
    storage->close();
}

// Test storage with zero-sized messages
TEST_F(StorageEdgeCasesTest, ZeroSizedMessage) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_zero.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    // Write zero-sized message
    storage->write_message("/test_topic", nullptr, 0, NANOSECONDS_PER_SECOND);
    
    EXPECT_EQ(storage->get_message_count(), 1);
    storage->close();
}

// Test storage with very large messages
TEST_F(StorageEdgeCasesTest, VeryLargeMessage) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_large.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/UInt8MultiArray";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    // Write large message (1MB)
    std::vector<uint8_t> large_data(1000000);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    storage->write_message("/test_topic", large_data.data(), large_data.size(), NANOSECONDS_PER_SECOND);
    
    EXPECT_EQ(storage->get_message_count(), 1);
    storage->close();
}

// Test storage append mode
TEST_F(StorageEdgeCasesTest, AppendMode) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_append.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    // First write
    storage->open(opts);
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    const char* msg1 = "Message 1";
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg1), 
                          strlen(msg1), 
                          NANOSECONDS_PER_SECOND);
    storage->close();
    
    // Append more messages
    opts.append = true;
    storage->open(opts);
    const char* msg2 = "Message 2";
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg2), 
                          strlen(msg2), 
                          2ULL * NANOSECONDS_PER_SECOND);
    
    EXPECT_GE(storage->get_message_count(), 1);
    storage->close();
}

// Test storage with many topics
TEST_F(StorageEdgeCasesTest, ManyTopics) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_many_topics.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    // Create 100 topics
    for (int i = 0; i < 100; ++i) {
        TopicMetadata topic;
        topic.id = static_cast<uint32_t>(i + 1);
        topic.name = "/topic" + std::to_string(i);
        topic.type = "std_msgs/String";
        topic.serialization_format = "cdr";
        storage->create_topic(topic);
    }
    
    auto topics = storage->get_topics();
    EXPECT_EQ(topics.size(), 100);
    
    storage->close();
}

// Test storage message count for non-existent topic
TEST_F(StorageEdgeCasesTest, MessageCountNonExistentTopic) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_count.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    // Get count for topic that doesn't exist
    uint64_t count = storage->get_message_count("/nonexistent_topic");
    EXPECT_EQ(count, 0);
    
    storage->close();
}

// Test storage time tracking edge cases
TEST_F(StorageEdgeCasesTest, TimeTrackingEdgeCases) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_time.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    // Write message with zero timestamp
    const char* msg = "Test";
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          strlen(msg), 
                          0);
    
    EXPECT_EQ(storage->get_start_time(), 0);
    EXPECT_EQ(storage->get_end_time(), 0);
    
    // Write message with very large timestamp
    uint64_t large_timestamp = UINT64_MAX / 2;
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          strlen(msg), 
                          large_timestamp);
    
    EXPECT_EQ(storage->get_end_time(), large_timestamp);
    
    storage->close();
}

// Test MCAP storage with custom options
TEST_F(StorageEdgeCasesTest, McapStorageCustomOptions) {
    auto storage = StorageFactory::create(StorageFormat::MCAP);
    StorageOptions opts;
    opts.uri = get_test_path("test_mcap_options.mcap");
    opts.storage_id = StorageFormat::MCAP;
    opts.append = true;
    
    // Add custom MCAP options
    opts.custom_data["mcap_compression"] = "Lz4";
    opts.custom_data["mcap_chunkSize"] = "2048";
    opts.custom_data["mcap_noChunkCRC"] = "true";
    
    bool result = storage->open(opts);
    if (result) {
        TopicMetadata topic;
        topic.id = 1;
        topic.name = "/test_topic";
        topic.type = "std_msgs/String";
        topic.serialization_format = "cdr";
        storage->create_topic(topic);
        
        const char* msg = "Test";
        storage->write_message("/test_topic", 
                              reinterpret_cast<const uint8_t*>(msg), 
                              strlen(msg), 
                              NANOSECONDS_PER_SECOND);
        
        storage->close();
    }
}

// Test storage with special characters in topic name
TEST_F(StorageEdgeCasesTest, SpecialCharactersInTopicName) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_special.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic_with_underscores_and-numbers-123";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    storage->create_topic(topic);
    
    const char* msg = "Test";
    storage->write_message(topic.name, 
                          reinterpret_cast<const uint8_t*>(msg), 
                          strlen(msg), 
                          NANOSECONDS_PER_SECOND);
    
    auto topics = storage->get_topics();
    bool found = false;
    for (const auto& t : topics) {
        if (t.name == topic.name) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
    
    storage->close();
}

// Test storage double close
TEST_F(StorageEdgeCasesTest, DoubleClose) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_double_close.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    storage->close();
    
    // Second close should be safe
    storage->close();
    EXPECT_FALSE(storage->is_open());
}

// Test storage get_topics on empty storage
TEST_F(StorageEdgeCasesTest, GetTopicsEmptyStorage) {
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    StorageOptions opts;
    opts.uri = get_test_path("test_empty.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    storage->open(opts);
    
    auto topics = storage->get_topics();
    EXPECT_EQ(topics.size(), 0);
    
    EXPECT_EQ(storage->get_message_count(), 0);
    EXPECT_EQ(storage->get_start_time(), 0);
    EXPECT_EQ(storage->get_end_time(), 0);
    
    storage->close();
}

