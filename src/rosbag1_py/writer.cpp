#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/compression.hpp"
#include "rosbag1_py/splitting.hpp"
#include <stdexcept>
#include <map>
#include <algorithm>

namespace rosbag1_py {

Writer::Writer() : splitter_(nullptr), is_open_(false), message_count_(0) {
}

Writer::~Writer() {
    if (is_open_) {
        close();
    }
}

void Writer::open(
    const StorageOptions& storage_options,
    const ConverterOptions& converter_options,
    const CompressionOptions& compression_options,
    const SplitOptions& split_options
) {
    if (is_open_) {
        throw std::runtime_error("Writer is already open");
    }
    
    storage_options_ = storage_options;
    // Ensure append mode is set for writing
    storage_options_.append = storage_options.append || true;
    converter_options_ = converter_options;
    compression_options_ = compression_options;
    split_options_ = split_options;
    base_uri_ = storage_options.uri;
    
    // Create splitter if splitting is enabled (any non-default value indicates splitting)
    if (split_options_.mode != SplitMode::SIZE || 
        split_options_.max_size != 1024ULL * 1024ULL * 1024ULL ||
        split_options_.max_duration != 300.0 ||
        split_options_.max_messages != 100000) {
        splitter_ = std::make_unique<BagSplitter>(split_options_);
    }
    
    // Create storage backend
    storage_ = StorageFactory::create(storage_options_.storage_id);
    
    if (!storage_->open(storage_options_)) {
        throw std::runtime_error("Failed to open storage");
    }
    
    is_open_ = true;
    message_count_ = 0;
}

void Writer::close() {
    if (!is_open_) {
        return;
    }
    
    if (storage_) {
        storage_->close();
        storage_.reset();
    }
    
    splitter_.reset();
    is_open_ = false;
}

bool Writer::is_open() const {
    return is_open_;
}

void Writer::create_topic(const TopicMetadata& topic) {
    if (!is_open_) {
        throw std::runtime_error("Writer is not open");
    }
    
    topics_[topic.name] = topic;
    storage_->create_topic(topic);
}

void Writer::write_message(
    const std::string& topic,
    const uint8_t* data,
    size_t data_size,
    uint64_t timestamp_ns
) {
    if (!is_open_) {
        throw std::runtime_error("Writer is not open");
    }
    
    if (topics_.find(topic) == topics_.end()) {
        throw std::runtime_error("Topic not registered: " + topic);
    }
    
    // Apply compression if enabled
    const uint8_t* write_data = data;
    size_t write_size = data_size;
    std::vector<uint8_t> compressed_data;
    
    if (compression_options_.compression_mode != CompressionMode::NONE) {
        std::string compression_name;
        switch (compression_options_.compression_mode) {
            case CompressionMode::BZ2: compression_name = "bz2"; break;
            case CompressionMode::LZ4: compression_name = "lz4"; break;
            case CompressionMode::ZSTD: compression_name = "zstd"; break;
            case CompressionMode::GZIP: compression_name = "gzip"; break;
            default: break;
        }
        
        if (!compression_name.empty()) {
            auto compressor = CompressionFactory::create(compression_name);
            if (compressor) {
                compressed_data = compressor->compress(
                    data,
                    data_size,
                    compression_options_.compression_level
                );
                write_data = compressed_data.data();
                write_size = compressed_data.size();
            }
        }
    }
    
    // Check if we need to split before writing
    if (splitter_) {
        double timestamp_sec = timestamp_ns / 1e9;
        // Check if split is needed BEFORE writing (using current size + new message size)
        if (splitter_->should_split(timestamp_sec, write_size)) {
            switch_to_next_file();
        }
    }
    
    // Write to storage
    storage_->write_message(topic, write_data, write_size, timestamp_ns);
    message_count_++;
    
    // Update splitter counters after writing
    if (splitter_) {
        double timestamp_sec = timestamp_ns / 1e9;
        splitter_->update_counters(write_size, timestamp_sec);
    }
}

void Writer::switch_to_next_file() {
    if (!splitter_ || !storage_) {
        return;
    }
    
    // Close current storage
    storage_->close();
    
    // Get next filename
    std::string storage_id_str = (storage_options_.storage_id == StorageFormat::MCAP) ? "mcap" : "bag";
    std::string next_filename = splitter_->get_next_filename(base_uri_, storage_id_str);
    
    // Update storage options with new filename
    storage_options_.uri = next_filename;
    storage_options_.append = false;  // New file, not append
    
    // Create new storage backend
    storage_ = StorageFactory::create(storage_options_.storage_id);
    
    if (!storage_->open(storage_options_)) {
        throw std::runtime_error("Failed to open next file for splitting: " + next_filename);
    }
    
    // Re-register all topics in the new file
    for (const auto& [name, topic] : topics_) {
        storage_->create_topic(topic);
    }
    
    // Reset splitter counters for new file
    splitter_->reset_counters();
}

std::map<std::string, std::string> Writer::get_metadata() const {
    std::map<std::string, std::string> metadata;
    metadata["message_count"] = std::to_string(message_count_);
    metadata["topic_count"] = std::to_string(topics_.size());
    return metadata;
}

} // namespace rosbag1_py

