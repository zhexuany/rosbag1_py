#include <gtest/gtest.h>
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/constants.hpp"
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class WriterComprehensiveTest : public ::testing::Test {
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

// Test writer with all storage formats
TEST_F(WriterComprehensiveTest, AllStorageFormats) {
    std::vector<StorageFormat> formats = {
        StorageFormat::MCAP,
        StorageFormat::ROSBAG,
        StorageFormat::SQLITE
    };
    
    for (const auto& format : formats) {
        Writer writer;
        StorageOptions storage_opts;
        storage_opts.uri = get_test_path("test_" + std::to_string(static_cast<int>(format)) + ".bag");
        storage_opts.storage_id = format;
        
        writer.open(storage_opts);
        EXPECT_TRUE(writer.is_open());
        
        TopicMetadata topic;
        topic.id = 1;
        topic.name = "/test_topic";
        topic.type = "std_msgs/String";
        topic.serialization_format = "cdr";
        writer.create_topic(topic);
        
        const char* msg = "Test message";
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            NANOSECONDS_PER_SECOND);
        
        writer.close();
    }
}

// Test file splitting - size based
TEST_F(WriterComprehensiveTest, FileSplittingSize) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_split_size");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    SplitOptions split_opts;
    split_opts.mode = SplitMode::SIZE;
    split_opts.max_size = 1024;  // 1KB
    
    writer.open(storage_opts, ConverterOptions(), CompressionOptions(), split_opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    writer.create_topic(topic);
    
    // Write messages that will trigger splitting
    std::vector<uint8_t> large_msg(600);  // Large enough to trigger split
    for (size_t i = 0; i < large_msg.size(); ++i) {
        large_msg[i] = static_cast<uint8_t>(i % 256);
    }
    
    for (int i = 0; i < 5; ++i) {
        writer.write_message("/test_topic",
                            large_msg.data(),
                            large_msg.size(),
                            static_cast<uint64_t>(i) * NANOSECONDS_PER_SECOND);
    }
    
    writer.close();
}

// Test file splitting - duration based
TEST_F(WriterComprehensiveTest, FileSplittingDuration) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_split_duration");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    SplitOptions split_opts;
    split_opts.mode = SplitMode::DURATION;
    split_opts.max_duration = 2.0;  // 2 seconds
    
    writer.open(storage_opts, ConverterOptions(), CompressionOptions(), split_opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    writer.create_topic(topic);
    
    const char* msg = "Test";
    // Write messages with increasing timestamps that will trigger split
    for (int i = 0; i < 10; ++i) {
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            static_cast<uint64_t>(i * 3) * NANOSECONDS_PER_SECOND);
    }
    
    writer.close();
}

// Test file splitting - message count based
TEST_F(WriterComprehensiveTest, FileSplittingMessageCount) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_split_count");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    SplitOptions split_opts;
    split_opts.mode = SplitMode::MESSAGE_COUNT;
    split_opts.max_messages = 3;
    
    writer.open(storage_opts, ConverterOptions(), CompressionOptions(), split_opts);
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    writer.create_topic(topic);
    
    const char* msg = "Test";
    for (int i = 0; i < 10; ++i) {
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            static_cast<uint64_t>(i) * NANOSECONDS_PER_SECOND);
    }
    
    writer.close();
}

// Test get_metadata
TEST_F(WriterComprehensiveTest, GetMetadata) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_metadata.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    writer.open(storage_opts);
    
    TopicMetadata topic1, topic2;
    topic1.id = 1;
    topic1.name = "/topic1";
    topic1.type = "std_msgs/String";
    topic1.serialization_format = "cdr";
    
    topic2.id = 2;
    topic2.name = "/topic2";
    topic2.type = "std_msgs/Int32";
    topic2.serialization_format = "cdr";
    
    writer.create_topic(topic1);
    writer.create_topic(topic2);
    
    const char* msg = "Test";
    writer.write_message("/topic1", reinterpret_cast<const uint8_t*>(msg), strlen(msg), NANOSECONDS_PER_SECOND);
    writer.write_message("/topic2", reinterpret_cast<const uint8_t*>(msg), strlen(msg), 2ULL * NANOSECONDS_PER_SECOND);
    
    auto metadata = writer.get_metadata();
    EXPECT_EQ(metadata["message_count"], "2");
    EXPECT_EQ(metadata["topic_count"], "2");
    
    writer.close();
}

// Test error handling - topic not registered
TEST_F(WriterComprehensiveTest, TopicNotRegistered) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_error.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    writer.open(storage_opts);
    
    const char* msg = "Test";
    EXPECT_THROW(
        writer.write_message("/unregistered_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            NANOSECONDS_PER_SECOND),
        std::runtime_error
    );
    
    writer.close();
}

// Test error handling - double open
TEST_F(WriterComprehensiveTest, DoubleOpen) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_double.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    writer.open(storage_opts);
    EXPECT_THROW(writer.open(storage_opts), std::runtime_error);
    
    writer.close();
}

// Test error handling - operations on closed writer
TEST_F(WriterComprehensiveTest, OperationsOnClosedWriter) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_closed.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    writer.open(storage_opts);
    writer.close();
    
    TopicMetadata topic;
    topic.id = 1;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    EXPECT_THROW(writer.create_topic(topic), std::runtime_error);
    
    const char* msg = "Test";
    EXPECT_THROW(
        writer.write_message("/test_topic",
                            reinterpret_cast<const uint8_t*>(msg),
                            strlen(msg),
                            NANOSECONDS_PER_SECOND),
        std::runtime_error
    );
}

// Test compression with different modes
TEST_F(WriterComprehensiveTest, CompressionModes) {
    std::vector<CompressionMode> modes = {
        CompressionMode::NONE,
        CompressionMode::LZ4,
        CompressionMode::ZSTD,
        CompressionMode::BZ2,
        CompressionMode::GZIP
    };
    
    for (const auto& mode : modes) {
        Writer writer;
        StorageOptions storage_opts;
        storage_opts.uri = get_test_path("test_comp_" + std::to_string(static_cast<int>(mode)) + ".mcap");
        storage_opts.storage_id = StorageFormat::MCAP;
        
        CompressionOptions comp_opts;
        comp_opts.compression_mode = mode;
        comp_opts.compression_level = 1;
        
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

// Test multiple topics and messages
TEST_F(WriterComprehensiveTest, MultipleTopicsAndMessages) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_multiple.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    writer.open(storage_opts);
    
    // Create multiple topics
    std::vector<std::string> topic_names = {"/topic1", "/topic2", "/topic3"};
    for (size_t i = 0; i < topic_names.size(); ++i) {
        TopicMetadata topic;
        topic.id = static_cast<uint32_t>(i + 1);
        topic.name = topic_names[i];
        topic.type = "std_msgs/String";
        topic.serialization_format = "cdr";
        writer.create_topic(topic);
    }
    
    // Write multiple messages to each topic
    for (const auto& topic_name : topic_names) {
        for (int i = 0; i < 5; ++i) {
            std::string msg = "Message " + std::to_string(i) + " for " + topic_name;
            writer.write_message(topic_name,
                                reinterpret_cast<const uint8_t*>(msg.c_str()),
                                msg.size(),
                                static_cast<uint64_t>(i) * NANOSECONDS_PER_SECOND);
        }
    }
    
    auto metadata = writer.get_metadata();
    EXPECT_EQ(metadata["message_count"], "15");
    EXPECT_EQ(metadata["topic_count"], "3");
    
    writer.close();
}

// Test error handling - invalid storage path
TEST_F(WriterComprehensiveTest, InvalidStoragePath) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = "/nonexistent/path/test.bag";
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    // This might succeed (creates directory) or fail depending on permissions
    // Just verify it doesn't crash
    try {
        writer.open(storage_opts);
        writer.close();
    } catch (...) {
        // Expected if path is invalid
    }
}

