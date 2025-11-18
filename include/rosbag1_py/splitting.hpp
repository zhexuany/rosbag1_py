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

#ifndef ROSBAG1_PY_SPLITTING_HPP
#define ROSBAG1_PY_SPLITTING_HPP

#include <string>
#include <cstdint>
#include <cstddef>

namespace rosbag1_py {

// Split mode
enum class SplitMode {
    SIZE,
    DURATION,
    MESSAGE_COUNT
};

// Split options
struct SplitOptions {
    SplitMode mode = SplitMode::SIZE;
    uint64_t max_size = 1024ULL * 1024ULL * 1024ULL;  // 1GB default
    double max_duration = 300.0;  // 5 minutes default
    uint64_t max_messages = 100000;  // 100k messages default
    std::string naming_pattern = "{basename}_{index:03d}";
};

// Bag splitter class
class BagSplitter {
public:
    BagSplitter(const SplitOptions& options);
    
    // Check if split is needed
    bool should_split(double current_time, size_t message_size);
    
    // Get next filename
    std::string get_next_filename(const std::string& base_uri, const std::string& storage_id);
    
    // Reset counters for new file
    void reset_counters();
    
    // Update tracking counters
    void update_counters(size_t message_size, double timestamp);
    
    // Get current file index
    uint32_t get_current_index() const { return current_file_index_; }
    
private:
    SplitOptions options_;
    uint32_t current_file_index_;
    uint64_t current_file_size_;
    double current_file_duration_;
    uint64_t current_file_messages_;
    double start_time_;
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_SPLITTING_HPP

