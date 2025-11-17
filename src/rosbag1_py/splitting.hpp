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
    bool should_split(double current_time, size_t message_size) const;
    
    // Get next filename
    std::string get_next_filename(const std::string& base_uri, const std::string& storage_id) const;
    
    // Reset counters for new file
    void reset_counters();
    
    // Update tracking counters
    void update_counters(size_t message_size, double timestamp);
    
    // Get current file index
    uint32_t get_current_index() const { return current_file_index_; }
    
private:
    SplitOptions options_;
    mutable uint32_t current_file_index_;
    mutable uint64_t current_file_size_;
    mutable double current_file_duration_;
    mutable uint64_t current_file_messages_;
    mutable double start_time_;
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_SPLITTING_HPP

