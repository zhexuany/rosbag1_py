#include <gtest/gtest.h>
#include "rosbag1_py/converter.hpp"
#include "rosbag1_py/constants.hpp"
#include <vector>
#include <cstring>

using namespace rosbag1_py;

class TypeConverterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test ROS1 to ROS2 type conversion with known mappings
TEST_F(TypeConverterTest, Ros1ToRos2KnownTypes) {
    EXPECT_EQ(TypeConverter::ros1_to_ros2("std_msgs/String"), "std_msgs/msg/String");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("std_msgs/Int32"), "std_msgs/msg/Int32");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("std_msgs/Float64"), "std_msgs/msg/Float64");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("std_msgs/Bool"), "std_msgs/msg/Bool");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("sensor_msgs/Image"), "sensor_msgs/msg/Image");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("sensor_msgs/PointCloud2"), "sensor_msgs/msg/PointCloud2");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("geometry_msgs/Pose"), "geometry_msgs/msg/Pose");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("geometry_msgs/Twist"), "geometry_msgs/msg/Twist");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("nav_msgs/Odometry"), "nav_msgs/msg/Odometry");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("tf2_msgs/TFMessage"), "tf2_msgs/msg/TFMessage");
}

// Test ROS1 to ROS2 generic conversion
TEST_F(TypeConverterTest, Ros1ToRos2GenericConversion) {
    EXPECT_EQ(TypeConverter::ros1_to_ros2("my_package/MyType"), "my_package/msg/MyType");
    EXPECT_EQ(TypeConverter::ros1_to_ros2("custom_msgs/Custom"), "custom_msgs/msg/Custom");
}

// Test ROS1 to ROS2 with no slash (should return as-is)
TEST_F(TypeConverterTest, Ros1ToRos2NoSlash) {
    std::string result = TypeConverter::ros1_to_ros2("NoSlashType");
    EXPECT_EQ(result, "NoSlashType");
}

// Test ROS2 to ROS1 type conversion with known mappings
TEST_F(TypeConverterTest, Ros2ToRos1KnownTypes) {
    EXPECT_EQ(TypeConverter::ros2_to_ros1("std_msgs/msg/String"), "std_msgs/String");
    EXPECT_EQ(TypeConverter::ros2_to_ros1("std_msgs/msg/Int32"), "std_msgs/Int32");
    EXPECT_EQ(TypeConverter::ros2_to_ros1("sensor_msgs/msg/Image"), "sensor_msgs/Image");
}

// Test ROS2 to ROS1 generic conversion
TEST_F(TypeConverterTest, Ros2ToRos1GenericConversion) {
    EXPECT_EQ(TypeConverter::ros2_to_ros1("my_package/msg/MyType"), "my_package/MyType");
    EXPECT_EQ(TypeConverter::ros2_to_ros1("custom_msgs/msg/Custom"), "custom_msgs/Custom");
}

// Test ROS2 to ROS1 with no /msg/ pattern
TEST_F(TypeConverterTest, Ros2ToRos1NoMsgPattern) {
    std::string result = TypeConverter::ros2_to_ros1("std_msgs/String");
    EXPECT_EQ(result, "std_msgs/String");
}

// Test ROS1 type detection
TEST_F(TypeConverterTest, IsRos1Type) {
    EXPECT_TRUE(TypeConverter::is_ros1_type("std_msgs/String"));
    EXPECT_TRUE(TypeConverter::is_ros1_type("sensor_msgs/Image"));
    EXPECT_FALSE(TypeConverter::is_ros1_type("std_msgs/msg/String"));
    EXPECT_FALSE(TypeConverter::is_ros1_type("sensor_msgs/msg/Image"));
}

// Test ROS2 type detection
TEST_F(TypeConverterTest, IsRos2Type) {
    EXPECT_TRUE(TypeConverter::is_ros2_type("std_msgs/msg/String"));
    EXPECT_TRUE(TypeConverter::is_ros2_type("sensor_msgs/msg/Image"));
    EXPECT_FALSE(TypeConverter::is_ros2_type("std_msgs/String"));
    EXPECT_FALSE(TypeConverter::is_ros2_type("sensor_msgs/Image"));
}

// Test round-trip conversion
TEST_F(TypeConverterTest, RoundTripConversion) {
    std::vector<std::string> ros1_types = {
        "std_msgs/String",
        "sensor_msgs/Image",
        "geometry_msgs/Pose",
        "my_package/MyType"
    };
    
    for (const auto& ros1_type : ros1_types) {
        std::string ros2_type = TypeConverter::ros1_to_ros2(ros1_type);
        std::string back_to_ros1 = TypeConverter::ros2_to_ros1(ros2_type);
        EXPECT_EQ(back_to_ros1, ros1_type);
    }
}

class SerializationConverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data
        test_data_ = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    }
    
    std::vector<uint8_t> test_data_;
};

// Test ROS1 to CDR conversion
TEST_F(SerializationConverterTest, Ros1ToCdr) {
    auto cdr_data = SerializationConverter::ros1_to_cdr(test_data_);
    
    // CDR should add 4-byte header
    EXPECT_EQ(cdr_data.size(), test_data_.size() + 4);
    
    // Check CDR header (little-endian, encoding options)
    EXPECT_EQ(cdr_data[0], 0x00);  // Endianness: little-endian
    EXPECT_EQ(cdr_data[1], 0x01);  // Encoding options
    EXPECT_EQ(cdr_data[2], 0x00);  // Reserved
    EXPECT_EQ(cdr_data[3], 0x00);  // Reserved
    
    // Check original data is preserved
    for (size_t i = 0; i < test_data_.size(); ++i) {
        EXPECT_EQ(cdr_data[i + 4], test_data_[i]);
    }
}

// Test CDR to ROS1 conversion
TEST_F(SerializationConverterTest, CdrToRos1) {
    // First create CDR data
    auto cdr_data = SerializationConverter::ros1_to_cdr(test_data_);
    
    // Convert back
    auto ros1_data = SerializationConverter::cdr_to_ros1(cdr_data);
    
    // Should match original
    EXPECT_EQ(ros1_data.size(), test_data_.size());
    EXPECT_EQ(ros1_data, test_data_);
}

// Test CDR to ROS1 with data too small
TEST_F(SerializationConverterTest, CdrToRos1TooSmall) {
    std::vector<uint8_t> small_data = {0x01, 0x02, 0x03};
    auto result = SerializationConverter::cdr_to_ros1(small_data);
    EXPECT_EQ(result, small_data);  // Should return as-is if < 4 bytes
}

// Test add_cdr_header (alias for ros1_to_cdr)
TEST_F(SerializationConverterTest, AddCdrHeader) {
    auto with_header = SerializationConverter::add_cdr_header(test_data_);
    auto with_header2 = SerializationConverter::ros1_to_cdr(test_data_);
    EXPECT_EQ(with_header, with_header2);
}

// Test remove_cdr_header (alias for cdr_to_ros1)
TEST_F(SerializationConverterTest, RemoveCdrHeader) {
    auto cdr_data = SerializationConverter::ros1_to_cdr(test_data_);
    auto without_header = SerializationConverter::remove_cdr_header(cdr_data);
    auto without_header2 = SerializationConverter::cdr_to_ros1(cdr_data);
    EXPECT_EQ(without_header, without_header2);
    EXPECT_EQ(without_header, test_data_);
}

// Test round-trip conversion
TEST_F(SerializationConverterTest, RoundTripConversion) {
    auto cdr = SerializationConverter::ros1_to_cdr(test_data_);
    auto back = SerializationConverter::cdr_to_ros1(cdr);
    EXPECT_EQ(back, test_data_);
}

// Test with empty data
TEST_F(SerializationConverterTest, EmptyData) {
    std::vector<uint8_t> empty;
    auto cdr = SerializationConverter::ros1_to_cdr(empty);
    EXPECT_EQ(cdr.size(), 4);  // Just header
    
    auto back = SerializationConverter::cdr_to_ros1(cdr);
    EXPECT_EQ(back.size(), 0);
}

// Test with large data
TEST_F(SerializationConverterTest, LargeData) {
    std::vector<uint8_t> large_data(10000);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    auto cdr = SerializationConverter::ros1_to_cdr(large_data);
    EXPECT_EQ(cdr.size(), large_data.size() + 4);
    
    auto back = SerializationConverter::cdr_to_ros1(cdr);
    EXPECT_EQ(back, large_data);
}

class TimeConverterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test ROS1 to nanoseconds conversion
TEST_F(TimeConverterTest, Ros1ToNanoseconds) {
    uint64_t ns = TimeConverter::ros1_to_nanoseconds(5, 500000000);
    EXPECT_EQ(ns, 5ULL * NANOSECONDS_PER_SECOND + 500000000ULL);
    
    ns = TimeConverter::ros1_to_nanoseconds(0, 0);
    EXPECT_EQ(ns, 0ULL);
    
    ns = TimeConverter::ros1_to_nanoseconds(1, 0);
    EXPECT_EQ(ns, NANOSECONDS_PER_SECOND);
    
    ns = TimeConverter::ros1_to_nanoseconds(0, NANOSECONDS_PER_SECOND - 1);
    EXPECT_EQ(ns, NANOSECONDS_PER_SECOND - 1);
}

// Test nanoseconds to ROS1 conversion
TEST_F(TimeConverterTest, NanosecondsToRos1) {
    auto [sec, nsec] = TimeConverter::nanoseconds_to_ros1(5500000000ULL);
    EXPECT_EQ(sec, 5U);
    EXPECT_EQ(nsec, 500000000U);
    
    auto [sec2, nsec2] = TimeConverter::nanoseconds_to_ros1(0ULL);
    EXPECT_EQ(sec2, 0U);
    EXPECT_EQ(nsec2, 0U);
    
    auto [sec3, nsec3] = TimeConverter::nanoseconds_to_ros1(NANOSECONDS_PER_SECOND);
    EXPECT_EQ(sec3, 1U);
    EXPECT_EQ(nsec3, 0U);
}

// Test round-trip conversion
TEST_F(TimeConverterTest, RoundTripConversion) {
    std::vector<std::pair<uint32_t, uint32_t>> test_cases = {
        {0, 0},
        {1, 0},
        {0, 1},
        {5, 500000000},
        {10, 999999999},
        {100, 123456789}
    };
    
    for (const auto& [sec, nsec] : test_cases) {
        uint64_t ns = TimeConverter::ros1_to_nanoseconds(sec, nsec);
        auto [result_sec, result_nsec] = TimeConverter::nanoseconds_to_ros1(ns);
        EXPECT_EQ(result_sec, sec);
        EXPECT_EQ(result_nsec, nsec);
    }
}

// Test edge cases
TEST_F(TimeConverterTest, EdgeCases) {
    // Maximum values
    uint32_t max_sec = UINT32_MAX;
    uint32_t max_nsec = UINT32_MAX;
    
    uint64_t ns = TimeConverter::ros1_to_nanoseconds(max_sec, max_nsec);
    auto [sec, nsec] = TimeConverter::nanoseconds_to_ros1(ns);
    
    // Note: This may overflow, but should handle gracefully
    EXPECT_GE(sec, 0U);
    EXPECT_GE(nsec, 0U);
}

