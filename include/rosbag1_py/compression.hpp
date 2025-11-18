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

#ifndef ROSBAG1_PY_COMPRESSION_HPP
#define ROSBAG1_PY_COMPRESSION_HPP

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace rosbag1_py {

// Abstract compression interface
class CompressionInterface {
public:
    virtual ~CompressionInterface() = default;
    
    virtual std::vector<uint8_t> compress(
        const uint8_t* data,
        size_t data_size,
        int level = -1
    ) = 0;
    
    virtual std::vector<uint8_t> decompress(
        const uint8_t* data,
        size_t data_size
    ) = 0;
    
    virtual std::string get_name() const = 0;
};

// Compression factory
class CompressionFactory {
public:
    static std::unique_ptr<CompressionInterface> create(const std::string& name);
    
    static bool is_available(const std::string& name);
    
    static void register_compression(
        const std::string& name,
        std::function<std::unique_ptr<CompressionInterface>()> factory
    );
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_COMPRESSION_HPP

