#include <gtest/gtest.h>
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/storage.hpp"
#include <cstring>

using namespace rosbag1_py;

class WriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(WriterTest, BasicCreation) {
    Writer writer;
    EXPECT_FALSE(writer.is_open());
}

TEST_F(WriterTest, OpenClose) {
    Writer writer;
    StorageOptions opts;
    opts.uri = "/tmp/test_bag.mcap";
    opts.storage_id = StorageFormat::MCAP;
    
    writer.open(opts);
    EXPECT_TRUE(writer.is_open());
    
    writer.close();
    EXPECT_FALSE(writer.is_open());
}

TEST_F(WriterTest, CreateTopic) {
    Writer writer;
    StorageOptions opts;
    opts.uri = "/tmp/test_bag.mcap";
    opts.storage_id = StorageFormat::MCAP;
    
    writer.open(opts);
    
    TopicMetadata topic;
    topic.id = 0;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    writer.create_topic(topic);
    EXPECT_TRUE(writer.is_open());
    
    writer.close();
}

TEST_F(WriterTest, WriteMessage) {
    Writer writer;
    StorageOptions opts;
    opts.uri = "/tmp/test_bag.mcap";
    opts.storage_id = StorageFormat::MCAP;
    
    writer.open(opts);
    
    TopicMetadata topic;
    topic.id = 0;
    topic.name = "/test_topic";
    topic.type = "std_msgs/String";
    topic.serialization_format = "cdr";
    
    writer.create_topic(topic);
    
    // Write test message
    const char* test_data = "Hello, World!";
    uint64_t timestamp_ns = 1000000000ULL;  // 1 second
    
    writer.write_message(
        "/test_topic",
        reinterpret_cast<const uint8_t*>(test_data),
        strlen(test_data),
        timestamp_ns
    );
    
    writer.close();
}

