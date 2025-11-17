#ifndef ROSBAG1_PY_STORAGE_HPP
#define ROSBAG1_PY_STORAGE_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>

namespace rosbag1_py {

// Storage format types
enum class StorageFormat {
    MCAP,
    ROSBAG,
    SQLITE
};

// Storage options
struct StorageOptions {
    std::string uri;
    StorageFormat storage_id = StorageFormat::MCAP;
    bool append = false;
    std::map<std::string, std::string> custom_data;
};

// Topic metadata
struct TopicMetadata {
    uint32_t id;
    std::string name;
    std::string type;
    std::string serialization_format;
    std::string md5sum;
    std::string definition;
    std::map<std::string, std::string> metadata;
};

// Abstract storage interface
class StorageInterface {
public:
    virtual ~StorageInterface() = default;
    
    virtual bool open(const StorageOptions& options) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    
    virtual void create_topic(const TopicMetadata& topic) = 0;
    virtual void write_message(
        const std::string& topic,
        const uint8_t* data,
        size_t data_size,
        uint64_t timestamp_ns
    ) = 0;
    
    virtual std::vector<TopicMetadata> get_topics() const = 0;
    virtual uint64_t get_message_count() const = 0;
    virtual uint64_t get_message_count(const std::string& topic_name) const = 0;
    virtual uint64_t get_start_time() const = 0;
    virtual uint64_t get_end_time() const = 0;
};

// Storage factory
class StorageFactory {
public:
    static std::unique_ptr<StorageInterface> create(StorageFormat format);
};

} // namespace rosbag1_py

#endif // ROSBAG1_PY_STORAGE_HPP

