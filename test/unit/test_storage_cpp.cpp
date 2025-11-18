#include <gtest/gtest.h>
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/constants.hpp"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class StorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = "/tmp/rosbag1_py_test_" + std::to_string(getpid());
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Cleanup test files
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
    std::string get_test_path(const std::string& filename) {
        return test_dir_ + "/" + filename;
    }
};

// Test RosbagStorage file handle persistence
TEST_F(StorageTest, RosbagStoragePersistentFileHandle) {
    StorageOptions opts;
    opts.uri = get_test_path("test_rosbag.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    ASSERT_TRUE(storage->open(opts));
    
    // Create topic
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    // Write multiple messages - should use same file handle
    const char* msg1 = "Message 1";
    const char* msg2 = "Message 2";
    const char* msg3 = "Message 3";
    
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg1), 
                          strlen(msg1), 1000000000ULL);
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg2), 
                          strlen(msg2), 2000000000ULL);
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg3), 
                          strlen(msg3), 3000000000ULL);
    
    EXPECT_EQ(storage->get_message_count(), 3);
    storage->close();
    
    // Verify file exists and has content
    std::ifstream file(opts.uri, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    EXPECT_GT(file_size, 0);
}

// Test SqliteStorage file handle persistence
TEST_F(StorageTest, SqliteStoragePersistentFileHandle) {
    StorageOptions opts;
    opts.uri = get_test_path("test_sqlite.db");
    opts.storage_id = StorageFormat::SQLITE;
    opts.append = true;
    
    auto storage = StorageFactory::create(StorageFormat::SQLITE);
    ASSERT_TRUE(storage->open(opts));
    
    // Create topic
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    // Write multiple messages
    const char* msg1 = "Message 1";
    const char* msg2 = "Message 2";
    
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg1), 
                          strlen(msg1), 1000000000ULL);
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg2), 
                          strlen(msg2), 2000000000ULL);
    
    EXPECT_EQ(storage->get_message_count(), 2);
    storage->close();
    
    // Verify file exists
    std::ifstream file(opts.uri, std::ios::binary);
    ASSERT_TRUE(file.is_open());
}

// Test MCAP storage
TEST_F(StorageTest, McapStorageBasic) {
    StorageOptions opts;
    opts.uri = get_test_path("test_mcap.mcap");
    opts.storage_id = StorageFormat::MCAP;
    opts.append = true;
    
    auto storage = StorageFactory::create(StorageFormat::MCAP);
    ASSERT_TRUE(storage->open(opts));
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    const char* msg = "Test message";
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          strlen(msg), 1000000000ULL);
    
    EXPECT_EQ(storage->get_message_count(), 1);
    storage->close();
}

// Test message counting per topic
TEST_F(StorageTest, MessageCountPerTopic) {
    StorageOptions opts;
    opts.uri = get_test_path("test_count.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    ASSERT_TRUE(storage->open(opts));
    
    TopicMetadata topic1, topic2;
    topic1.id = 1;
    topic1.name = "/topic1";
    topic1.type = "std_msgs/String";
    topic1.serialization_format = "cdr";
    
    topic2.id = 2;
    topic2.name = "/topic2";
    topic2.type = "std_msgs/Int32";
    topic2.serialization_format = "cdr";
    
    storage->create_topic(topic1);
    storage->create_topic(topic2);
    
    const char* msg = "Test";
    storage->write_message("/topic1", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          4, 1000000000ULL);
    storage->write_message("/topic1", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          4, 2000000000ULL);
    storage->write_message("/topic2", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          4, 3000000000ULL);
    
    EXPECT_EQ(storage->get_message_count("/topic1"), 2);
    EXPECT_EQ(storage->get_message_count("/topic2"), 1);
    EXPECT_EQ(storage->get_message_count(), 3);
    
    storage->close();
}

// Test time tracking
TEST_F(StorageTest, TimeTracking) {
    StorageOptions opts;
    opts.uri = get_test_path("test_time.bag");
    opts.storage_id = StorageFormat::ROSBAG;
    opts.append = true;
    
    auto storage = StorageFactory::create(StorageFormat::ROSBAG);
    ASSERT_TRUE(storage->open(opts));
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    storage->create_topic(topic);
    
    uint64_t start_time = 1000000000ULL;
    uint64_t end_time = 5000000000ULL;
    
    const char* msg = "Test";
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          4, start_time);
    storage->write_message("/test_topic", 
                          reinterpret_cast<const uint8_t*>(msg), 
                          4, end_time);
    
    EXPECT_EQ(storage->get_start_time(), start_time);
    EXPECT_EQ(storage->get_end_time(), end_time);
    
    storage->close();
}

