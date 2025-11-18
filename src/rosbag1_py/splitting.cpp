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

#include "rosbag1_py/splitting.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#ifdef __has_include
    #if __has_include(<filesystem>)
        #include <filesystem>
        namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

namespace rosbag1_py {

BagSplitter::BagSplitter(const SplitOptions& options)
    : options_(options),
      current_file_index_(0),
      current_file_size_(0),
      current_file_duration_(0.0),
      current_file_messages_(0),
      start_time_(0.0)
{
}

bool BagSplitter::should_split(double current_time, size_t message_size) {
    if (start_time_ == 0.0) {
        return false;
    }
    
    switch (options_.mode) {
        case SplitMode::SIZE:
            return (current_file_size_ + message_size) >= options_.max_size;
        
        case SplitMode::DURATION:
            return (current_time - start_time_) >= options_.max_duration;
        
        case SplitMode::MESSAGE_COUNT:
            return current_file_messages_ >= options_.max_messages;
        
        default:
            return false;
    }
}

std::string BagSplitter::get_next_filename(
    const std::string& base_uri,
    const std::string& storage_id
) {
    fs::path path(base_uri);
    std::string basename = path.stem().string();
    std::string dirname = path.parent_path().string();
    if (dirname.empty()) {
        dirname = ".";
    }
    
    // Apply naming pattern
    std::string filename = options_.naming_pattern;
    
    // Replace {basename}
    size_t basename_pos = filename.find("{basename}");
    if (basename_pos != std::string::npos) {
        filename.replace(basename_pos, 10, basename);
    }
    
    // Replace {index:03d}
    size_t index_pos = filename.find("{index:03d}");
    if (index_pos != std::string::npos) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(3) << current_file_index_;
        filename.replace(index_pos, 11, oss.str());
    }
    
    // Add extension
    std::string ext = (storage_id == "mcap") ? ".mcap" : ".bag";
    std::string full_path = (fs::path(dirname) / (filename + ext)).string();
    
    current_file_index_++;
    return full_path;
}

void BagSplitter::reset_counters() {
    current_file_size_ = 0;
    current_file_messages_ = 0;
    start_time_ = 0.0;
    current_file_duration_ = 0.0;
}

void BagSplitter::update_counters(size_t message_size, double timestamp) {
    current_file_size_ += message_size;
    current_file_messages_++;
    
    if (start_time_ == 0.0) {
        start_time_ = timestamp;
    }
    
    current_file_duration_ = timestamp - start_time_;
}

} // namespace rosbag1_py

