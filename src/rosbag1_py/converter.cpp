// Copyright 2025 Zhexuan Yang
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rosbag1_py/converter.hpp"
#include "rosbag1_py/constants.hpp"
#include <map>
#include <sstream>
#include <algorithm>

namespace rosbag1_py {

// ROS1 to ROS2 type mapping
const std::map<std::string, std::string> TypeConverter::ros1_to_ros2_map_ = {
    {"std_msgs/String", "std_msgs/msg/String"},
    {"std_msgs/Int32", "std_msgs/msg/Int32"},
    {"std_msgs/Float64", "std_msgs/msg/Float64"},
    {"std_msgs/Bool", "std_msgs/msg/Bool"},
    {"sensor_msgs/Image", "sensor_msgs/msg/Image"},
    {"sensor_msgs/PointCloud2", "sensor_msgs/msg/PointCloud2"},
    {"geometry_msgs/Pose", "geometry_msgs/msg/Pose"},
    {"geometry_msgs/Twist", "geometry_msgs/msg/Twist"},
    {"nav_msgs/Odometry", "nav_msgs/msg/Odometry"},
    {"tf2_msgs/TFMessage", "tf2_msgs/msg/TFMessage"},
};

// ROS2 to ROS1 type mapping (reverse)
const std::map<std::string, std::string> TypeConverter::ros2_to_ros1_map_ = []() {
    std::map<std::string, std::string> result;
    for (const auto& [ros1, ros2] : TypeConverter::ros1_to_ros2_map_) {
        result[ros2] = ros1;
    }
    return result;
}();

std::string TypeConverter::ros1_to_ros2(const std::string& type_name) {
    auto it = ros1_to_ros2_map_.find(type_name);
    if (it != ros1_to_ros2_map_.end()) {
        return it->second;
    }
    
    // Generic conversion: package/Type -> package/msg/Type
    size_t slash_pos = type_name.find_last_of('/');
    if (slash_pos != std::string::npos) {
        std::string package = type_name.substr(0, slash_pos);
        std::string type = type_name.substr(slash_pos + 1);
        return package + "/msg/" + type;
    }
    
    return type_name;
}

std::string TypeConverter::ros2_to_ros1(const std::string& type_name) {
    auto it = ros2_to_ros1_map_.find(type_name);
    if (it != ros2_to_ros1_map_.end()) {
        return it->second;
    }
    
    // Generic conversion: package/msg/Type -> package/Type
    size_t msg_pos = type_name.find("/msg/");
    if (msg_pos != std::string::npos) {
        std::string package = type_name.substr(0, msg_pos);
        std::string type = type_name.substr(msg_pos + 5);
        return package + "/" + type;
    }
    
    return type_name;
}

bool TypeConverter::is_ros1_type(const std::string& type_name) {
    return type_name.find("/msg/") == std::string::npos;
}

bool TypeConverter::is_ros2_type(const std::string& type_name) {
    return type_name.find("/msg/") != std::string::npos;
}

// Serialization converter implementation
std::vector<uint8_t> SerializationConverter::ros1_to_cdr(const std::vector<uint8_t>& data) {
    // CDR encapsulation header: 4 bytes
    // Byte 0: Endianness (0 = big-endian, 1 = little-endian)
    // Byte 1: Encoding options
    // Bytes 2-3: Reserved
    std::vector<uint8_t> cdr_data;
    cdr_data.reserve(data.size() + 4);
    
    // Little-endian CDR header
    cdr_data.push_back(0x00);  // Endianness: little-endian
    cdr_data.push_back(0x01);  // Encoding options
    cdr_data.push_back(0x00);  // Reserved
    cdr_data.push_back(0x00);  // Reserved
    
    // Append original data
    cdr_data.insert(cdr_data.end(), data.begin(), data.end());
    
    return cdr_data;
}

std::vector<uint8_t> SerializationConverter::cdr_to_ros1(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return data;  // Not a valid CDR message
    }
    
    // Remove CDR header (first 4 bytes)
    return std::vector<uint8_t>(data.begin() + 4, data.end());
}

std::vector<uint8_t> SerializationConverter::add_cdr_header(const std::vector<uint8_t>& data) {
    return ros1_to_cdr(data);
}

std::vector<uint8_t> SerializationConverter::remove_cdr_header(const std::vector<uint8_t>& data) {
    return cdr_to_ros1(data);
}

// Time converter implementation
uint64_t TimeConverter::ros1_to_nanoseconds(uint32_t sec, uint32_t nsec) {
    return static_cast<uint64_t>(sec) * NANOSECONDS_PER_SECOND + static_cast<uint64_t>(nsec);
}

std::pair<uint32_t, uint32_t> TimeConverter::nanoseconds_to_ros1(uint64_t nanoseconds) {
    uint32_t sec = static_cast<uint32_t>(nanoseconds / NANOSECONDS_PER_SECOND);
    uint32_t nsec = static_cast<uint32_t>(nanoseconds % NANOSECONDS_PER_SECOND);
    return {sec, nsec};
}

} // namespace rosbag1_py

