#ifndef ROSBAG1_PY_SERIALIZATION_HPP
#define ROSBAG1_PY_SERIALIZATION_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace rosbag1_py {

// Serialize ROS1 message
std::vector<uint8_t> serialize_message(
    const std::string& message_type,
    const uint8_t* message_data,
    size_t message_size
);

// Deserialize ROS1 message
std::vector<uint8_t> deserialize_message(
    const std::string& message_type,
    const uint8_t* serialized_data,
    size_t serialized_size
);

} // namespace rosbag1_py

#endif // ROSBAG1_PY_SERIALIZATION_HPP

