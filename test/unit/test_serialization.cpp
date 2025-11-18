#include <gtest/gtest.h>
#include "rosbag1_py/serialization.hpp"
#include <vector>
#include <cstring>

using namespace rosbag1_py;

class SerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data
        test_data_.resize(100);
        for (size_t i = 0; i < test_data_.size(); ++i) {
            test_data_[i] = static_cast<uint8_t>(i % 256);
        }
    }
    
    std::vector<uint8_t> test_data_;
};

// Test serialize_message with valid data
TEST_F(SerializationTest, SerializeMessage) {
    auto serialized = serialize_message("std_msgs/String", 
                                        test_data_.data(), 
                                        test_data_.size());
    
    EXPECT_EQ(serialized.size(), test_data_.size());
    EXPECT_EQ(memcmp(serialized.data(), test_data_.data(), test_data_.size()), 0);
}

// Test serialize_message with empty data
TEST_F(SerializationTest, SerializeMessageEmpty) {
    std::vector<uint8_t> empty;
    auto serialized = serialize_message("std_msgs/String", 
                                        empty.data(), 
                                        0);
    
    EXPECT_EQ(serialized.size(), 0);
}

// Test serialize_message with different message types
TEST_F(SerializationTest, SerializeMessageDifferentTypes) {
    const char* msg_data = "test message";
    size_t msg_size = strlen(msg_data);
    
    auto serialized1 = serialize_message("std_msgs/String", 
                                          reinterpret_cast<const uint8_t*>(msg_data), 
                                          msg_size);
    auto serialized2 = serialize_message("std_msgs/Int32", 
                                          reinterpret_cast<const uint8_t*>(msg_data), 
                                          msg_size);
    
    // Currently, serialization just copies data regardless of type
    EXPECT_EQ(serialized1.size(), msg_size);
    EXPECT_EQ(serialized2.size(), msg_size);
}

// Test serialize_message with large data
TEST_F(SerializationTest, SerializeMessageLarge) {
    std::vector<uint8_t> large_data(100000);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    auto serialized = serialize_message("sensor_msgs/Image", 
                                        large_data.data(), 
                                        large_data.size());
    
    EXPECT_EQ(serialized.size(), large_data.size());
    EXPECT_EQ(memcmp(serialized.data(), large_data.data(), large_data.size()), 0);
}

// Test deserialize_message with valid data
TEST_F(SerializationTest, DeserializeMessage) {
    auto deserialized = deserialize_message("std_msgs/String", 
                                           test_data_.data(), 
                                           test_data_.size());
    
    EXPECT_EQ(deserialized.size(), test_data_.size());
    EXPECT_EQ(memcmp(deserialized.data(), test_data_.data(), test_data_.size()), 0);
}

// Test deserialize_message with empty data
TEST_F(SerializationTest, DeserializeMessageEmpty) {
    std::vector<uint8_t> empty;
    auto deserialized = deserialize_message("std_msgs/String", 
                                            empty.data(), 
                                            0);
    
    EXPECT_EQ(deserialized.size(), 0);
}

// Test deserialize_message with different message types
TEST_F(SerializationTest, DeserializeMessageDifferentTypes) {
    const char* msg_data = "test message";
    size_t msg_size = strlen(msg_data);
    
    auto deserialized1 = deserialize_message("std_msgs/String", 
                                             reinterpret_cast<const uint8_t*>(msg_data), 
                                             msg_size);
    auto deserialized2 = deserialize_message("std_msgs/Int32", 
                                             reinterpret_cast<const uint8_t*>(msg_data), 
                                             msg_size);
    
    // Currently, deserialization just copies data regardless of type
    EXPECT_EQ(deserialized1.size(), msg_size);
    EXPECT_EQ(deserialized2.size(), msg_size);
}

// Test round-trip serialization/deserialization
TEST_F(SerializationTest, RoundTrip) {
    auto serialized = serialize_message("std_msgs/String", 
                                       test_data_.data(), 
                                       test_data_.size());
    
    auto deserialized = deserialize_message("std_msgs/String", 
                                           serialized.data(), 
                                           serialized.size());
    
    EXPECT_EQ(deserialized.size(), test_data_.size());
    EXPECT_EQ(memcmp(deserialized.data(), test_data_.data(), test_data_.size()), 0);
}

// Test with single byte
TEST_F(SerializationTest, SingleByte) {
    uint8_t single_byte = 0x42;
    
    auto serialized = serialize_message("std_msgs/UInt8", &single_byte, 1);
    EXPECT_EQ(serialized.size(), 1);
    EXPECT_EQ(serialized[0], single_byte);
    
    auto deserialized = deserialize_message("std_msgs/UInt8", serialized.data(), 1);
    EXPECT_EQ(deserialized.size(), 1);
    EXPECT_EQ(deserialized[0], single_byte);
}

// Test with pattern data
TEST_F(SerializationTest, PatternData) {
    std::vector<uint8_t> pattern = {0x00, 0xFF, 0x00, 0xFF, 0xAA, 0x55};
    
    auto serialized = serialize_message("std_msgs/UInt8MultiArray", 
                                       pattern.data(), 
                                       pattern.size());
    EXPECT_EQ(serialized, pattern);
    
    auto deserialized = deserialize_message("std_msgs/UInt8MultiArray", 
                                           serialized.data(), 
                                           serialized.size());
    EXPECT_EQ(deserialized, pattern);
}

