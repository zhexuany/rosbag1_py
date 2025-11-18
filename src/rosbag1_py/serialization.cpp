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

#include "rosbag1_py/serialization.hpp"
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cstring>

namespace rosbag1_py {

// Serialize ROS1 message
// Note: In practice, ROS1 messages are serialized in Python using msg.serialize(buff)
// This C++ function is provided for cases where serialization needs to happen in C++
// The data passed here is typically already serialized from Python
std::vector<uint8_t> serialize_message(
    const std::string& /* message_type */,
    const uint8_t* message_data,
    size_t message_size
) {
    // ROS1 serialization format:
    // - Messages are serialized in network byte order (big-endian)
    // - Each field is serialized according to its type
    // - Variable-length fields (strings, arrays) are prefixed with length
    
    // For now, we assume the data is already properly serialized from Python
    // In a full implementation, we would:
    // 1. Look up the message type definition
    // 2. Use ros::serialization::OStream to serialize each field
    // 3. Handle endianness conversion if needed
    
    std::vector<uint8_t> serialized(message_data, message_data + message_size);
    return serialized;
}

// Deserialize ROS1 message
// Note: In practice, ROS1 messages are deserialized in Python using msg.deserialize(buff)
// This C++ function is provided for cases where deserialization needs to happen in C++
std::vector<uint8_t> deserialize_message(
    const std::string& /* message_type */,
    const uint8_t* serialized_data,
    size_t serialized_size
) {
    // ROS1 deserialization format:
    // - Messages are deserialized from network byte order (big-endian)
    // - Each field is deserialized according to its type
    // - Variable-length fields (strings, arrays) read length prefix first
    
    // For now, we return the data as-is (assuming it needs to be deserialized in Python)
    // In a full implementation, we would:
    // 1. Look up the message type definition
    // 2. Use ros::serialization::IStream to deserialize each field
    // 3. Handle endianness conversion if needed
    // 4. Return the deserialized message data
    
    std::vector<uint8_t> deserialized(serialized_data, serialized_data + serialized_size);
    return deserialized;
}

} // namespace rosbag1_py

