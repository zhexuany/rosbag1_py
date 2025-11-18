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

#ifndef ROSBAG1_PY_CONVERTER_HPP
#define ROSBAG1_PY_CONVERTER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace rosbag1_py {

// Type converter for ROS1/ROS2 compatibility
class TypeConverter {
public:
    // Convert ROS1 type name to ROS2 type name
    static std::string ros1_to_ros2(const std::string& type_name);
    
    // Convert ROS2 type name to ROS1 type name
    static std::string ros2_to_ros1(const std::string& type_name);
    
    // Check if type is ROS1 format
    static bool is_ros1_type(const std::string& type_name);
    
    // Check if type is ROS2 format
    static bool is_ros2_type(const std::string& type_name);
    
private:
    static const std::map<std::string, std::string> ros1_to_ros2_map_;
    static const std::map<std::string, std::string> ros2_to_ros1_map_;
};

// Serialization format converter
class SerializationConverter {
public:
    // Convert ROS1 serialization to CDR
    static std::vector<uint8_t> ros1_to_cdr(const std::vector<uint8_t>& data);
    
    // Convert CDR to ROS1 serialization
    static std::vector<uint8_t> cdr_to_ros1(const std::vector<uint8_t>& data);
    
    // Add CDR encapsulation header
    static std::vector<uint8_t> add_cdr_header(const std::vector<uint8_t>& data);
    
    // Remove CDR encapsulation header
    static std::vector<uint8_t> remove_cdr_header(const std::vector<uint8_t>& data);
};

// Time converter
class TimeConverter {
public:
    // Convert ROS1 time (sec, nsec) to nanoseconds
    static uint64_t ros1_to_nanoseconds(uint32_t sec, uint32_t nsec);
    
    // Convert nanoseconds to ROS1 time (sec, nsec)
    static std::pair<uint32_t, uint32_t> nanoseconds_to_ros1(uint64_t nanoseconds);
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_CONVERTER_HPP

