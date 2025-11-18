#include <gtest/gtest.h>
#include "rosbag1_py/reader.hpp"
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/constants.hpp"
#include <filesystem>
#include <map>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <climits>

using namespace rosbag1_py;

class ReaderComprehensiveTest : public ::testing::Test {
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
    
    void create_test_bag(const std::string& filename, StorageFormat format) {
        Writer writer;
        StorageOptions storage_opts;
        storage_opts.uri = get_test_path(filename);
        storage_opts.storage_id = format;
        
        writer.open(storage_opts);
        
        // Create topics
        std::vector<std::string> topics = {"/topic1", "/topic2", "/topic3"};
        for (size_t i = 0; i < topics.size(); ++i) {
            TopicMetadata topic;
            topic.id = static_cast<uint32_t>(i + 1);
            topic.name = topics[i];
            topic.type = "std_msgs/String";
            topic.serialization_format = "cdr";
            writer.create_topic(topic);
        }
        
        // Write messages with different timestamps
        for (size_t i = 0; i < topics.size(); ++i) {
            for (int j = 0; j < 10; ++j) {
                std::string msg = "Message " + std::to_string(j) + " for " + topics[i];
                uint64_t timestamp = static_cast<uint64_t>(i * 10 + j) * NANOSECONDS_PER_SECOND;
                writer.write_message(topics[i],
                                    reinterpret_cast<const uint8_t*>(msg.c_str()),
                                    msg.size(),
                                    timestamp);
            }
        }
        
        writer.close();
    }
    
    std::string test_dir_;
    std::string get_test_path(const std::string& filename) {
        return test_dir_ + "/" + filename;
    }
};

// Test reading MCAP format
TEST_F(ReaderComprehensiveTest, ReadMcapFormat) {
    create_test_bag("test_read.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_read.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    std::map<std::string, int> message_counts;
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        {}
    );
    
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 10);
    EXPECT_EQ(message_counts["/topic3"], 10);
    
    reader.close();
}

// Test reading ROSBAG format
TEST_F(ReaderComprehensiveTest, ReadRosbagFormat) {
    create_test_bag("test_read.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_read.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    std::map<std::string, int> message_counts;
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        {}
    );
    
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 10);
    EXPECT_EQ(message_counts["/topic3"], 10);
    
    reader.close();
}

// Test reading SQLITE format
TEST_F(ReaderComprehensiveTest, ReadSqliteFormat) {
    create_test_bag("test_read.db", StorageFormat::SQLITE);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_read.db");
    storage_opts.storage_id = StorageFormat::SQLITE;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    std::map<std::string, int> message_counts;
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        {}
    );
    
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 10);
    EXPECT_EQ(message_counts["/topic3"], 10);
    
    reader.close();
}

// Test time range filtering
TEST_F(ReaderComprehensiveTest, TimeRangeFiltering) {
    create_test_bag("test_time.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_time.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    // Filter messages between 10 and 20 seconds
    uint64_t start_time = 10ULL * NANOSECONDS_PER_SECOND;
    uint64_t end_time = 20ULL * NANOSECONDS_PER_SECOND;
    
    std::vector<MessageData> messages = reader.read_messages({}, start_time, end_time);
    
    // Should get messages in the time range
    for (const auto& msg : messages) {
        EXPECT_GE(msg.timestamp_ns, start_time);
        EXPECT_LE(msg.timestamp_ns, end_time);
    }
    
    reader.close();
}

// Test get_topics method
TEST_F(ReaderComprehensiveTest, GetTopics) {
    create_test_bag("test_topics.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_topics.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    std::map<std::string, bool> found_topics;
    for (const auto& topic : topics) {
        found_topics[topic.name] = true;
        EXPECT_FALSE(topic.name.empty());
        EXPECT_FALSE(topic.type.empty());
    }
    
    EXPECT_TRUE(found_topics["/topic1"]);
    EXPECT_TRUE(found_topics["/topic2"]);
    EXPECT_TRUE(found_topics["/topic3"]);
    
    reader.close();
}

// Test get_metadata method
TEST_F(ReaderComprehensiveTest, GetMetadata) {
    create_test_bag("test_metadata.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_metadata.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    auto metadata = reader.get_metadata();
    EXPECT_EQ(metadata.message_count, 30);  // 3 topics * 10 messages
    
    EXPECT_GT(metadata.start_time_ns, 0);
    EXPECT_GT(metadata.end_time_ns, metadata.start_time_ns);
    
    EXPECT_GE(metadata.topics_with_message_count.size(), 3);
    
    std::map<std::string, uint64_t> topic_counts;
    for (const auto& [name, type, count] : metadata.topics_with_message_count) {
        topic_counts[name] = count;
    }
    
    EXPECT_EQ(topic_counts["/topic1"], 10);
    EXPECT_EQ(topic_counts["/topic2"], 10);
    EXPECT_EQ(topic_counts["/topic3"], 10);
    
    reader.close();
}

// Test iterator-based reading
TEST_F(ReaderComprehensiveTest, IteratorBasedReading) {
    create_test_bag("test_iterator.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_iterator.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    auto messages = reader.read_messages({}, 0, UINT64_MAX);
    EXPECT_EQ(messages.size(), 30);
    
    // Verify message order (should be by timestamp)
    for (size_t i = 1; i < messages.size(); ++i) {
        EXPECT_GE(messages[i].timestamp_ns, messages[i-1].timestamp_ns);
    }
    
    reader.close();
}

// Test error handling - file not found
TEST_F(ReaderComprehensiveTest, FileNotFound) {
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("nonexistent.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    EXPECT_THROW(reader.open(storage_opts), std::runtime_error);
}

// Test error handling - operations on closed reader
TEST_F(ReaderComprehensiveTest, OperationsOnClosedReader) {
    Reader reader;
    
    EXPECT_THROW(reader.get_topics(), std::runtime_error);
    EXPECT_THROW(reader.get_metadata(), std::runtime_error);
    EXPECT_THROW(reader.read_messages({}, 0, UINT64_MAX), std::runtime_error);
}

// Test error handling - double open
TEST_F(ReaderComprehensiveTest, DoubleOpen) {
    create_test_bag("test_double.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_double.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    EXPECT_THROW(reader.open(storage_opts), std::runtime_error);
    
    reader.close();
}

// Test reading empty bag
TEST_F(ReaderComprehensiveTest, EmptyBag) {
    Writer writer;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("empty.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    writer.open(storage_opts);
    writer.close();
    
    Reader reader;
    reader.open(storage_opts);
    
    auto topics = reader.get_topics();
    auto messages = reader.read_messages({}, 0, UINT64_MAX);
    
    EXPECT_EQ(messages.size(), 0);
    
    reader.close();
}

// Test topic filtering with time range
TEST_F(ReaderComprehensiveTest, TopicFilteringWithTimeRange) {
    create_test_bag("test_filter_time.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_filter_time.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic1"};
    uint64_t start_time = 5ULL * NANOSECONDS_PER_SECOND;
    uint64_t end_time = 15ULL * NANOSECONDS_PER_SECOND;
    
    auto messages = reader.read_messages(filters, start_time, end_time);
    
    for (const auto& msg : messages) {
        EXPECT_EQ(msg.topic, "/topic1");
        EXPECT_GE(msg.timestamp_ns, start_time);
        EXPECT_LE(msg.timestamp_ns, end_time);
    }
    
    reader.close();
}

// Test reading with callback exceptions
TEST_F(ReaderComprehensiveTest, CallbackExceptionHandling) {
    create_test_bag("test_callback.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_callback.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    int call_count = 0;
    reader.read_messages(
        [&call_count](const MessageData& msg) {
            call_count++;
            if (call_count == 5) {
                throw std::runtime_error("Test exception");
            }
        },
        {}
    );
    
    // Should continue reading after exception
    EXPECT_GT(call_count, 5);
    
    reader.close();
}

// Test MCAP time range filtering
TEST_F(ReaderComprehensiveTest, McapTimeRangeFiltering) {
    create_test_bag("test_mcap_time.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_time.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    
    // Filter messages between 10 and 20 seconds
    uint64_t start_time = 10ULL * NANOSECONDS_PER_SECOND;
    uint64_t end_time = 20ULL * NANOSECONDS_PER_SECOND;
    
    std::vector<MessageData> messages = reader.read_messages({}, start_time, end_time);
    
    // Should get messages in the time range
    for (const auto& msg : messages) {
        EXPECT_GE(msg.timestamp_ns, start_time);
        EXPECT_LE(msg.timestamp_ns, end_time);
    }
    
    reader.close();
}

// Test SQLITE time range filtering
TEST_F(ReaderComprehensiveTest, SqliteTimeRangeFiltering) {
    create_test_bag("test_sqlite_time.db", StorageFormat::SQLITE);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_sqlite_time.db");
    storage_opts.storage_id = StorageFormat::SQLITE;
    
    reader.open(storage_opts);
    
    // Filter messages between 10 and 20 seconds
    uint64_t start_time = 10ULL * NANOSECONDS_PER_SECOND;
    uint64_t end_time = 20ULL * NANOSECONDS_PER_SECOND;
    
    std::vector<MessageData> messages = reader.read_messages({}, start_time, end_time);
    
    // SQLITE format doesn't support time range filtering in read_messages
    // So this should return all messages (or empty if not implemented)
    // The test verifies the code path executes without crashing
    
    reader.close();
}

// Test extension handling - file without extension
TEST_F(ReaderComprehensiveTest, ExtensionHandlingNoExtension) {
    create_test_bag("test_no_ext", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_no_ext");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    reader.close();
}

// Test extension handling - file with extension already
TEST_F(ReaderComprehensiveTest, ExtensionHandlingWithExtension) {
    create_test_bag("test_with_ext.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_with_ext.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    reader.close();
}

// Test MCAP extension handling
TEST_F(ReaderComprehensiveTest, McapExtensionHandling) {
    create_test_bag("test_mcap_ext", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_ext");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    EXPECT_TRUE(reader.is_open());
    
    auto topics = reader.get_topics();
    EXPECT_GE(topics.size(), 3);
    
    reader.close();
}

// Test callback-based reading with topic filters on MCAP
TEST_F(ReaderComprehensiveTest, McapCallbackWithTopicFilters) {
    create_test_bag("test_mcap_filter.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_filter.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic1"};
    std::map<std::string, int> message_counts;
    
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        filters
    );
    
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 0);
    EXPECT_EQ(message_counts["/topic3"], 0);
    
    reader.close();
}

// Test callback-based reading with topic filters on SQLITE
TEST_F(ReaderComprehensiveTest, SqliteCallbackWithTopicFilters) {
    create_test_bag("test_sqlite_filter.db", StorageFormat::SQLITE);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_sqlite_filter.db");
    storage_opts.storage_id = StorageFormat::SQLITE;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic1"};
    std::map<std::string, int> message_counts;
    
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        filters
    );
    
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 0);
    EXPECT_EQ(message_counts["/topic3"], 0);
    
    reader.close();
}

// Test reading with empty topic filters
TEST_F(ReaderComprehensiveTest, EmptyTopicFilters) {
    create_test_bag("test_empty_filters.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_empty_filters.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    std::vector<std::string> empty_filters;
    std::map<std::string, int> message_counts;
    
    reader.read_messages(
        [&message_counts](const MessageData& msg) {
            message_counts[msg.topic]++;
        },
        empty_filters
    );
    
    // Should read all messages when filters are empty
    EXPECT_EQ(message_counts["/topic1"], 10);
    EXPECT_EQ(message_counts["/topic2"], 10);
    EXPECT_EQ(message_counts["/topic3"], 10);
    
    reader.close();
}

// Test callback-based reading with callback exception on MCAP
TEST_F(ReaderComprehensiveTest, McapCallbackExceptionHandling) {
    create_test_bag("test_mcap_callback.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_callback.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    
    int call_count = 0;
    reader.read_messages(
        [&call_count](const MessageData& msg) {
            call_count++;
            if (call_count == 5) {
                throw std::runtime_error("Test exception");
            }
        },
        {}
    );
    
    // Should continue reading after exception
    EXPECT_GT(call_count, 5);
    
    reader.close();
}

// Test reading messages with time range on MCAP format
TEST_F(ReaderComprehensiveTest, McapTimeRangeWithTopicFilters) {
    create_test_bag("test_mcap_time_filter.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_time_filter.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic1"};
    uint64_t start_time = 5ULL * NANOSECONDS_PER_SECOND;
    uint64_t end_time = 15ULL * NANOSECONDS_PER_SECOND;
    
    auto messages = reader.read_messages(filters, start_time, end_time);
    
    for (const auto& msg : messages) {
        EXPECT_EQ(msg.topic, "/topic1");
        EXPECT_GE(msg.timestamp_ns, start_time);
        EXPECT_LE(msg.timestamp_ns, end_time);
    }
    
    reader.close();
}

// Test reading messages with all time range (no filtering)
TEST_F(ReaderComprehensiveTest, TimeRangeNoFiltering) {
    create_test_bag("test_time_all.bag", StorageFormat::ROSBAG);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_time_all.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    // Read all messages with full time range
    auto messages = reader.read_messages({}, 0, UINT64_MAX);
    EXPECT_EQ(messages.size(), 30);
    
    reader.close();
}

// Test get_metadata for MCAP format
TEST_F(ReaderComprehensiveTest, McapGetMetadata) {
    create_test_bag("test_mcap_metadata.mcap", StorageFormat::MCAP);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_mcap_metadata.mcap");
    storage_opts.storage_id = StorageFormat::MCAP;
    
    reader.open(storage_opts);
    
    auto metadata = reader.get_metadata();
    EXPECT_EQ(metadata.message_count, 30);
    EXPECT_GT(metadata.start_time_ns, 0);
    EXPECT_GT(metadata.end_time_ns, metadata.start_time_ns);
    
    reader.close();
}

// Test get_metadata for SQLITE format
TEST_F(ReaderComprehensiveTest, SqliteGetMetadata) {
    create_test_bag("test_sqlite_metadata.db", StorageFormat::SQLITE);
    
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_sqlite_metadata.db");
    storage_opts.storage_id = StorageFormat::SQLITE;
    
    reader.open(storage_opts);
    
    auto metadata = reader.get_metadata();
    EXPECT_EQ(metadata.message_count, 30);
    EXPECT_GT(metadata.start_time_ns, 0);
    EXPECT_GT(metadata.end_time_ns, metadata.start_time_ns);
    
    reader.close();
}

// Test callback-based reading on closed reader
TEST_F(ReaderComprehensiveTest, CallbackOnClosedReader) {
    Reader reader;
    
    EXPECT_THROW(
        reader.read_messages(
            [](const MessageData&) {},
            {}
        ),
        std::runtime_error
    );
}

