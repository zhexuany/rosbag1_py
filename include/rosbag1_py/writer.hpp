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

#ifndef ROSBAG1_PY_WRITER_HPP
#define ROSBAG1_PY_WRITER_HPP

#include "storage.hpp"
#include "splitting.hpp"
#include "compression.hpp"
#include <string>
#include <memory>
#include <cstdint>
#include <map>

namespace rosbag1_py {

// Converter options
struct ConverterOptions {
    std::string input_serialization_format = "cdr";
    std::string output_serialization_format = "cdr";
};

// Compression mode
enum class CompressionMode {
    NONE,
    BZ2,
    LZ4,
    ZSTD,
    GZIP
};

// Compression options
struct CompressionOptions {
    CompressionMode compression_mode = CompressionMode::NONE;
    int compression_level = -1;  // -1 = default
    size_t compression_queue_size = 1000;
    size_t compression_threads = 4;
};

// Writer class
class Writer {
public:
    Writer();
    ~Writer();
    
    // Open bag for writing
    void open(
        const StorageOptions& storage_options,
        const ConverterOptions& converter_options = ConverterOptions(),
        const CompressionOptions& compression_options = CompressionOptions(),
        const SplitOptions& split_options = SplitOptions()
    );
    
    // Close bag
    void close();
    
    // Check if bag is open
    bool is_open() const;
    
    // Create topic
    void create_topic(const TopicMetadata& topic);
    
    // Write message
    void write_message(
        const std::string& topic,
        const uint8_t* data,
        size_t data_size,
        uint64_t timestamp_ns
    );
    
    // Get metadata
    std::map<std::string, std::string> get_metadata() const;
    
private:
    std::unique_ptr<StorageInterface> storage_;
    StorageOptions storage_options_;
    ConverterOptions converter_options_;
    CompressionOptions compression_options_;
    SplitOptions split_options_;
    std::unique_ptr<BagSplitter> splitter_;
    std::unique_ptr<CompressionInterface> compressor_;
    std::map<std::string, TopicMetadata> topics_;
    bool is_open_;
    uint64_t message_count_;
    std::string base_uri_;
    
    // Helper to switch to next file when splitting
    void switch_to_next_file();
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_WRITER_HPP

