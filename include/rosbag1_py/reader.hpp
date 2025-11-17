#ifndef ROSBAG1_PY_READER_HPP
#define ROSBAG1_PY_READER_HPP

#include "storage.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace rosbag1_py {

// Message data structure
struct MessageData {
    std::string topic;
    std::vector<uint8_t> data;
    uint64_t timestamp_ns;
    std::string type;
    std::string serialization_format;
};

// Message callback function type
using MessageCallback = std::function<void(const MessageData&)>;

// Reader class
class Reader {
public:
    Reader();
    ~Reader();
    
    // Open bag for reading
    void open(const StorageOptions& storage_options);
    
    // Close bag
    void close();
    
    // Check if bag is open
    bool is_open() const;
    
    // Get all topics
    std::vector<TopicMetadata> get_topics() const;
    
    // Read messages (callback-based)
    void read_messages(
        const MessageCallback& callback,
        const std::vector<std::string>& topic_filters = {}
    );
    
    // Read messages (iterator-based)
    std::vector<MessageData> read_messages(
        const std::vector<std::string>& topic_filters = {},
        uint64_t start_time_ns = 0,
        uint64_t end_time_ns = UINT64_MAX
    );
    
    // Get bag metadata
    struct BagMetadata {
        uint64_t message_count;
        uint64_t start_time_ns;
        uint64_t end_time_ns;
        std::vector<std::tuple<std::string, std::string, uint64_t>> topics_with_message_count;
    };
    
    BagMetadata get_metadata() const;
    
private:
    std::unique_ptr<StorageInterface> storage_;
    StorageOptions storage_options_;
    bool is_open_;
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_READER_HPP

