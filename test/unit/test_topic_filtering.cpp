#include <gtest/gtest.h>
#include "rosbag1_py/reader.hpp"
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/constants.hpp"
#include <filesystem>
#include <unordered_set>
#include <map>
#include <cstring>
#include <climits>
#include <unistd.h>
#include <sys/types.h>

using namespace rosbag1_py;

class TopicFilteringTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "/tmp/rosbag1_py_test_" + std::to_string(getpid());
        std::filesystem::create_directories(test_dir_);
        
        // Create a test bag with multiple topics
        create_test_bag();
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    void create_test_bag() {
        Writer writer;
        StorageOptions storage_opts;
        storage_opts.uri = get_test_path("test_filter.bag");
        storage_opts.storage_id = StorageFormat::ROSBAG;
        
        writer.open(storage_opts);
        
        // Create multiple topics
        std::vector<std::string> topics = {"/topic1", "/topic2", "/topic3"};
        for (size_t i = 0; i < topics.size(); ++i) {
            TopicMetadata topic;
            topic.id = static_cast<uint32_t>(i + 1);
            topic.name = topics[i];
            topic.type = "std_msgs/String";
            topic.serialization_format = "cdr";
            writer.create_topic(topic);
        }
        
        // Write messages to each topic
        for (size_t i = 0; i < topics.size(); ++i) {
            std::string msg = "Message for " + topics[i];
            for (int j = 0; j < 5; ++j) {
                writer.write_message(topics[i],
                                    reinterpret_cast<const uint8_t*>(msg.c_str()),
                                    msg.size(),
                                    static_cast<uint64_t>(i * 10 + j) * NANOSECONDS_PER_SECOND);
            }
        }
        
        writer.close();
    }
    
    std::string test_dir_;
    std::string get_test_path(const std::string& filename) {
        return test_dir_ + "/" + filename;
    }
};

// Test topic filtering with unordered_set (performance test)
TEST_F(TopicFilteringTest, TopicFilterPerformance) {
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_filter.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    // Test with single topic filter
    std::vector<std::string> filters = {"/topic1"};
    
    int message_count = 0;
    reader.read_messages(
        [&message_count](const MessageData& msg) {
            message_count++;
            EXPECT_EQ(msg.topic, "/topic1");
        },
        filters
    );
    
    EXPECT_EQ(message_count, 5);  // 5 messages for topic1
    reader.close();
}

// Test multiple topic filters
TEST_F(TopicFilteringTest, MultipleTopicFilters) {
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_filter.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic1", "/topic3"};
    
    std::map<std::string, int> topic_counts;
    reader.read_messages(
        [&topic_counts](const MessageData& msg) {
            topic_counts[msg.topic]++;
        },
        filters
    );
    
    EXPECT_EQ(topic_counts["/topic1"], 5);
    EXPECT_EQ(topic_counts["/topic3"], 5);
    EXPECT_EQ(topic_counts.count("/topic2"), 0);  // Should be filtered out
    reader.close();
}

// Test empty filter (should read all topics)
TEST_F(TopicFilteringTest, EmptyFilter) {
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_filter.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {};
    std::map<std::string, int> topic_counts;
    
    reader.read_messages(
        [&topic_counts](const MessageData& msg) {
            topic_counts[msg.topic]++;
        },
        filters
    );
    
    EXPECT_EQ(topic_counts["/topic1"], 5);
    EXPECT_EQ(topic_counts["/topic2"], 5);
    EXPECT_EQ(topic_counts["/topic3"], 5);
    reader.close();
}

// Test iterator-based read with filters
TEST_F(TopicFilteringTest, IteratorBasedFilter) {
    Reader reader;
    StorageOptions storage_opts;
    storage_opts.uri = get_test_path("test_filter.bag");
    storage_opts.storage_id = StorageFormat::ROSBAG;
    
    reader.open(storage_opts);
    
    std::vector<std::string> filters = {"/topic2"};
    auto messages = reader.read_messages(filters, 0, UINT64_MAX);
    
    EXPECT_EQ(messages.size(), 5);
    for (const auto& msg : messages) {
        EXPECT_EQ(msg.topic, "/topic2");
    }
    
    reader.close();
}

