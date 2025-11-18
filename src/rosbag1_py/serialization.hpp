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

