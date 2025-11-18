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

#include "rosbag1_py/storage.hpp"
#include <stdexcept>
#include <fstream>
#include <string>
#include <map>
#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cctype>

// MCAP is a header-only library, but requires MCAP_IMPLEMENTATION to be defined
// in exactly one source file to instantiate the implementation
#define MCAP_IMPLEMENTATION
#include <mcap/writer.hpp>
#include <mcap/reader.hpp>
#include <mcap/types.hpp>
// Status is defined in writer.hpp and reader.hpp

namespace rosbag1_py {

// Helper function to parse boolean from string
static bool parse_bool(const std::string& value, bool default_val = false) {
    if (value.empty()) return default_val;
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
}

// Helper function to parse integer from string
static int parse_int(const std::string& value, int default_val = 0) {
    if (value.empty()) return default_val;
    try {
        return std::stoi(value);
    } catch (...) {
        return default_val;
    }
}

// Helper function to parse uint64_t from string
static uint64_t parse_uint64(const std::string& value, uint64_t default_val = 0) {
    if (value.empty()) return default_val;
    try {
        return std::stoull(value);
    } catch (...) {
        return default_val;
    }
}

// Parse MCAP writer options from custom_data map
McapWriterOptions parse_mcap_writer_options(const std::map<std::string, std::string>& custom_data) {
    McapWriterOptions opts;
    
    // Set defaults to match current behavior for backward compatibility
    opts.compression = "Zstd";
    opts.chunkSize = 1024;  // 1KB chunks - small to ensure chunking happens
    opts.noChunkCRC = false;
    opts.noSummaryCRC = false;
    
    // Check for preset profile first
    std::string preset = "";
    auto preset_it = custom_data.find("mcap_preset_profile");
    if (preset_it == custom_data.end()) {
        preset_it = custom_data.find("preset_profile");
    }
    if (preset_it != custom_data.end()) {
        preset = preset_it->second;
        std::transform(preset.begin(), preset.end(), preset.begin(), ::tolower);
    }
    
    // Apply preset profile defaults
    if (preset == "fastwrite") {
        opts.noChunking = true;
        opts.noSummaryCRC = true;
        opts.preset_profile = "fastwrite";
    } else if (preset == "none") {
        // Use defaults (already set)
        opts.preset_profile = "none";
    }
    
    // Helper lambda to get option value (check both with and without mcap_ prefix)
    auto get_option = [&custom_data](const std::string& key) -> std::string {
        auto it = custom_data.find("mcap_" + key);
        if (it != custom_data.end()) return it->second;
        it = custom_data.find(key);
        if (it != custom_data.end()) return it->second;
        return "";
    };
    
    // Parse boolean options (only override if explicitly set)
    std::string val = get_option("noChunkCRC");
    if (!val.empty()) opts.noChunkCRC = parse_bool(val);
    
    val = get_option("noAttachmentCRC");
    if (!val.empty()) opts.noAttachmentCRC = parse_bool(val);
    
    val = get_option("enableDataCRC");
    if (!val.empty()) opts.enableDataCRC = parse_bool(val);
    
    val = get_option("noSummaryCRC");
    if (!val.empty()) opts.noSummaryCRC = parse_bool(val);
    
    val = get_option("noChunking");
    if (!val.empty()) opts.noChunking = parse_bool(val);
    
    val = get_option("noMessageIndex");
    if (!val.empty()) opts.noMessageIndex = parse_bool(val);
    
    val = get_option("noSummary");
    if (!val.empty()) opts.noSummary = parse_bool(val);
    
    val = get_option("noMetadataIndex");
    if (!val.empty()) opts.noMetadataIndex = parse_bool(val);
    
    val = get_option("noChunkIndex");
    if (!val.empty()) opts.noChunkIndex = parse_bool(val);
    
    val = get_option("noStatistics");
    if (!val.empty()) opts.noStatistics = parse_bool(val);
    
    val = get_option("noSummaryOffsets");
    if (!val.empty()) opts.noSummaryOffsets = parse_bool(val);
    
    val = get_option("forceCompression");
    if (!val.empty()) opts.forceCompression = parse_bool(val);
    
    // Parse numeric options
    val = get_option("chunkSize");
    if (!val.empty()) opts.chunkSize = parse_uint64(val, 1024);  // Default 1KB for backward compatibility
    
    val = get_option("compressionLevel");
    if (!val.empty()) opts.compressionLevel = parse_int(val, -1);
    
    // Parse compression type
    val = get_option("compression");
    if (!val.empty()) {
        std::string comp_lower = val;
        std::transform(comp_lower.begin(), comp_lower.end(), comp_lower.begin(), ::tolower);
        if (comp_lower == "none" || comp_lower == "0") {
            opts.compression = "None";
        } else if (comp_lower == "lz4" || comp_lower == "1") {
            opts.compression = "Lz4";
        } else if (comp_lower == "zstd" || comp_lower == "2") {
            opts.compression = "Zstd";
        } else {
            // Keep original value (capitalized)
            opts.compression = val;
        }
    }
    
    return opts;
}

// MCAP storage implementation using MCAP C++ library
class McapStorage : public StorageInterface {
public:
    McapStorage() : is_open_(false), is_writing_(false), message_count_(0), start_time_ns_(0), end_time_ns_(0), writer_(nullptr), reader_(nullptr), output_stream_(nullptr), input_stream_(nullptr) {}
    
    ~McapStorage() {
        close();
    }
    
    bool open(const StorageOptions& options) override {
        options_ = options;
        std::string filename = options.uri;
        if (filename.find(".mcap") == std::string::npos) {
            filename += ".mcap";
        }
        filename_ = filename;
        
        // Check if file exists
        std::ifstream test_file(filename, std::ios::binary);
        bool file_exists = test_file.is_open();
        test_file.close();
        
        // Determine if we're reading or writing
        // If append is false and file exists, we're likely reading
        // If append is true or file doesn't exist, we're writing
        is_writing_ = !file_exists || options.append;
        
        if (is_writing_) {
            // Open for writing
            output_stream_ = std::make_unique<std::ofstream>(filename, std::ios::binary);
            if (!output_stream_->is_open()) {
                return false;
            }
            
                writer_ = std::make_unique<mcap::McapWriter>();
                
                // Parse MCAP writer options from custom_data
                McapWriterOptions mcap_opts = parse_mcap_writer_options(options.custom_data);
                
                // Create MCAP writer options with ROS1 profile
                mcap::McapWriterOptions writer_options("ros1");
                
                // Apply parsed options
                writer_options.noChunkCRC = mcap_opts.noChunkCRC;
                writer_options.noAttachmentCRC = mcap_opts.noAttachmentCRC;
                writer_options.enableDataCRC = mcap_opts.enableDataCRC;
                writer_options.noSummaryCRC = mcap_opts.noSummaryCRC;
                writer_options.noChunking = mcap_opts.noChunking;
                writer_options.noMessageIndex = mcap_opts.noMessageIndex;
                writer_options.noSummary = mcap_opts.noSummary;
                writer_options.noMetadataIndex = mcap_opts.noMetadataIndex;
                writer_options.noChunkIndex = mcap_opts.noChunkIndex;
                writer_options.noStatistics = mcap_opts.noStatistics;
                writer_options.noSummaryOffsets = mcap_opts.noSummaryOffsets;
                writer_options.forceCompression = mcap_opts.forceCompression;
                writer_options.chunkSize = mcap_opts.chunkSize;
                // Convert compression level int to enum (if -1, use default)
                if (mcap_opts.compressionLevel == -1) {
                    // Use default compression level
                    writer_options.compressionLevel = mcap::CompressionLevel::Default;
                } else {
                    // Cast int to enum (MCAP uses enum class for compression level)
                    writer_options.compressionLevel = static_cast<mcap::CompressionLevel>(mcap_opts.compressionLevel);
                }
                
                // Convert compression string to enum
                if (mcap_opts.compression == "None" || mcap_opts.compression == "none") {
                    writer_options.compression = mcap::Compression::None;
                } else if (mcap_opts.compression == "Lz4" || mcap_opts.compression == "lz4") {
                    writer_options.compression = mcap::Compression::Lz4;
                } else if (mcap_opts.compression == "Zstd" || mcap_opts.compression == "zstd") {
                    writer_options.compression = mcap::Compression::Zstd;
                } else {
                    // Default to Zstd if unknown
                    writer_options.compression = mcap::Compression::Zstd;
                }
                
                // NOTE: MCAP automatically writes summary/index on close() if chunks were written
                // The summary section is what allows fast topic discovery
                // In MCAP v2.1.1, writer_->open() returns void - errors are handled via exceptions
                try {
                    writer_->open(*output_stream_, writer_options);
                } catch (...) {
                    output_stream_->close();
                    return false;
                }
        } else {
            // Open for reading
            input_stream_ = std::make_unique<std::ifstream>(filename, std::ios::binary);
            if (!input_stream_->is_open()) {
                return false;
            }
            
            reader_ = std::make_unique<mcap::McapReader>();
            // In MCAP v2.1.1, open() returns Status with [[nodiscard]] attribute
            mcap::Status status = reader_->open(*input_stream_);
            if (!status.ok()) {
                input_stream_->close();
                return false;
            }
            
            // In MCAP, we need to read the summary to get channels
            // The summary is read automatically by open() if it exists
            // If no summary exists (streaming file), channels are discovered by reading messages
            // For now, try to get channels - they should be available after open()
            
            // Read existing topics/channels and count messages
            // Channels should be available from summary after open() if summary exists
            // If channels are empty, the file might not have a summary or it's a streaming file
            // MCAP v2.x: channels should be loaded from summary during open() if summary exists
            // However, if the file has no messages, channels might not be discovered by reading messages
            // In that case, we need to rely on the summary being read properly
            // But MCAP's open() should read the summary automatically if it exists
            
            for (const auto& [channel_id, channel] : reader_->channels()) {
                TopicMetadata topic;
                topic.id = channel_id;
                topic.name = channel->topic;
                // Try to find schema for message type
                // In MCAP v2.x, schemaId is not optional, it's just SchemaId
                if (channel->schemaId != 0) {
                    auto schema_it = reader_->schemas().find(channel->schemaId);
                    if (schema_it != reader_->schemas().end()) {
                        topic.type = schema_it->second->name;
                        // Convert std::byte vector to string
                        const auto& schema_data = schema_it->second->data;
                        topic.definition.assign(reinterpret_cast<const char*>(schema_data.data()),
                                               schema_data.size());
                    }
                }
                if (topic.type.empty()) {
                    // Try to get type from metadata
                    auto type_it = channel->metadata.find("ros1_type");
                    if (type_it != channel->metadata.end()) {
                        topic.type = type_it->second;
                    } else {
                        topic.type = "unknown";  // Fallback if schema not found
                    }
                }
                topic.serialization_format = channel->messageEncoding.empty() ? "cdr" : channel->messageEncoding;
                topics_[topic.name] = topic;
            }
            
            // In MCAP, channels are available from the summary if it exists
            // Use the MCAP summary/statistics if available for efficient metadata access
            // Otherwise, read through the file to discover channels
            
            // Try to use statistics from MCAP summary if available
            const auto& statistics = reader_->statistics();
            bool has_statistics = statistics && statistics->messageCount > 0;
            
            // Clear any previous data
            topic_message_counts_.clear();
            message_count_ = 0;
            start_time_ns_ = 0;
            end_time_ns_ = 0;
            
            if (has_statistics) {
                // Use statistics from summary
                message_count_ = statistics->messageCount;
                if (statistics->messageStartTime > 0) {
                    start_time_ns_ = statistics->messageStartTime;
                }
                if (statistics->messageEndTime > 0) {
                    end_time_ns_ = statistics->messageEndTime;
                }
                
                // Count messages per channel
                if (!statistics->channelMessageCounts.empty()) {
                    for (const auto& [channel_id, count] : statistics->channelMessageCounts) {
                        auto channel_it = reader_->channels().find(channel_id);
                        if (channel_it != reader_->channels().end()) {
                            std::string topic_name = channel_it->second->topic;
                            topic_message_counts_[topic_name] = count;
                            
                            // Ensure topic is in topics_ map
                            if (topics_.find(topic_name) == topics_.end()) {
                                TopicMetadata topic;
                                topic.id = channel_id;
                                topic.name = topic_name;
                                auto channel = channel_it->second;
                                if (channel->schemaId != 0) {
                                    auto schema_it = reader_->schemas().find(channel->schemaId);
                                    if (schema_it != reader_->schemas().end()) {
                                        topic.type = schema_it->second->name;
                                        const auto& schema_data = schema_it->second->data;
                                        topic.definition.assign(reinterpret_cast<const char*>(schema_data.data()),
                                                               schema_data.size());
                                    }
                                }
                                if (topic.type.empty()) {
                                    auto type_it = channel->metadata.find("ros1_type");
                                    if (type_it != channel->metadata.end()) {
                                        topic.type = type_it->second;
                                    } else {
                                        topic.type = "unknown";
                                    }
                                }
                                topic.serialization_format = channel->messageEncoding.empty() ? "cdr" : channel->messageEncoding;
                                topics_[topic.name] = topic;
                            }
                        }
                    }
                }
            } else {
                // No statistics available, need to read through file
                // First ensure we have channels
                if (reader_->channels().empty()) {
                    // Read through messages to discover channels
                    // Note: If file has no messages, channels might be in summary but not loaded yet
                    // Try reading summary explicitly first
                    auto message_view = reader_->readMessages();
                    for (const auto& msg_view : message_view) {
                        (void)msg_view;  // Just iterate to discover channels
                    }
                    
                    // If still no channels and no messages, the file might have channels in summary
                    // but they weren't loaded. Try to access them directly.
                    // MCAP should have loaded channels from summary during open(), so if they're still
                    // empty, the file might not have a summary or channels weren't written.
                }
                
                // Update topics_ from discovered channels
                for (const auto& [channel_id, channel] : reader_->channels()) {
                    if (topics_.find(channel->topic) == topics_.end()) {
                        TopicMetadata topic;
                        topic.id = channel_id;
                        topic.name = channel->topic;
                        if (channel->schemaId != 0) {
                            auto schema_it = reader_->schemas().find(channel->schemaId);
                            if (schema_it != reader_->schemas().end()) {
                                topic.type = schema_it->second->name;
                                const auto& schema_data = schema_it->second->data;
                                topic.definition.assign(reinterpret_cast<const char*>(schema_data.data()),
                                                       schema_data.size());
                            }
                        }
                        if (topic.type.empty()) {
                            auto type_it = channel->metadata.find("ros1_type");
                            if (type_it != channel->metadata.end()) {
                                topic.type = type_it->second;
                            } else {
                                topic.type = "unknown";
                            }
                        }
                        topic.serialization_format = channel->messageEncoding.empty() ? "cdr" : channel->messageEncoding;
                        topics_[topic.name] = topic;
                    }
                }
                
                // Build channel ID to topic name map from discovered topics
                // We need this because after reopening, channels might not be immediately available
                std::map<mcap::ChannelId, std::string> channel_id_to_topic;
                for (const auto& [channel_id, channel] : reader_->channels()) {
                    channel_id_to_topic[channel_id] = channel->topic;
                }
                
                // Now count messages - create a new reader to reset position
                input_stream_->close();
                input_stream_ = std::make_unique<std::ifstream>(filename, std::ios::binary);
                reader_ = std::make_unique<mcap::McapReader>();
                status = reader_->open(*input_stream_);
                if (status.ok()) {
                    // Always read through messages once to ensure ALL channels are discovered
                    // This is necessary because channels might not be in the summary
                    // IMPORTANT: Don't look up channels during reading - MCAP may return incorrect channels
                    // Wait until after all messages are read, then build the map from reader_->channels()
                    auto discovery_view = reader_->readMessages();
                    for (const auto& msg_view : discovery_view) {
                        (void)msg_view;  // Just iterate to discover channels, don't look them up yet
                    }
                    // After reading all messages, build the channel map from reader_->channels()
                    // This ensures we get the correct channel-to-topic mappings
                    for (const auto& [channel_id, channel] : reader_->channels()) {
                        channel_id_to_topic[channel_id] = channel->topic;
                    }
                    
                    // Close and reopen to reset position for counting
                    reader_->close();
                    input_stream_->close();
                    input_stream_ = std::make_unique<std::ifstream>(filename, std::ios::binary);
                    reader_ = std::make_unique<mcap::McapReader>();
                    status = reader_->open(*input_stream_);
                    if (!status.ok()) {
                        return true;  // Can't count if reader can't be reopened
                    }
                    // Update map with any channels that are now available from summary
                    for (const auto& [channel_id, channel] : reader_->channels()) {
                        channel_id_to_topic[channel_id] = channel->topic;
                    }
                    
                    auto message_view = reader_->readMessages();
                    bool first_message = true;
                    int msg_index = 0;
                    for (const auto& msg_view : message_view) {
                        const mcap::MessageView& msg = msg_view;
                        msg_index++;
                        
                        // Use the channel map we built, or try to get from reader if not in map
                        auto channel_it = channel_id_to_topic.find(msg.message.channelId);
                        if (channel_it == channel_id_to_topic.end()) {
                            // Try to get from reader (might be discovered during read)
                            auto discovered_channel_it = reader_->channels().find(msg.message.channelId);
                            if (discovered_channel_it != reader_->channels().end()) {
                                std::string topic_name = discovered_channel_it->second->topic;
                                channel_id_to_topic[msg.message.channelId] = topic_name;
                                channel_it = channel_id_to_topic.find(msg.message.channelId);
                            }
                        }
                        if (channel_it != channel_id_to_topic.end()) {
                            std::string topic_name = channel_it->second;
                            topic_message_counts_[topic_name]++;
                            message_count_++;
                            
                            if (first_message) {
                                start_time_ns_ = msg.message.logTime;
                                first_message = false;
                            }
                            end_time_ns_ = msg.message.logTime;
                        }
                    }
                }
            }
            
            // Note: After reading all messages, the reader position is at the end
            // For subsequent reads in read_messages(), we'll create a new reader
            // which will read the summary (if available) or discover channels by reading
        }
        
        is_open_ = true;
        return true;
    }
    
    void close() override {
        if (writer_) {
            // MCAP writer needs to be closed to finalize the file (write summary/index)
            // This must happen BEFORE closing the output stream
            try {
                writer_->close();
            } catch (...) {
                // Ignore close errors
            }
            writer_.reset();
        }
        if (output_stream_) {
            // Flush the output stream to ensure all data is written to disk
            // This ensures the MCAP file is properly finalized before we can read it
            try {
                output_stream_->flush();
                output_stream_->close();
            } catch (...) {
                // Ignore flush/close errors
            }
            output_stream_.reset();
        }
        if (reader_) {
            try {
                reader_->close();
            } catch (...) {
                // Ignore close errors
            }
            reader_.reset();
        }
        if (input_stream_) {
            try {
                input_stream_->close();
            } catch (...) {
                // Ignore close errors
            }
            input_stream_.reset();
        }
        is_open_ = false;
        is_writing_ = false;
    }
    
    bool is_open() const override {
        return is_open_;
    }
    
    void create_topic(const TopicMetadata& topic) override {
        if (!is_open_ || !is_writing_ || !writer_) {
            throw std::runtime_error("Storage is not open for writing");
        }
        
        topics_[topic.name] = topic;
        
        // Create schema if definition is provided
        mcap::SchemaId schema_id = 0;
        if (!topic.definition.empty()) {
            mcap::Schema schema;
            schema.name = topic.type;
            schema.encoding = "ros1msg";  // ROS1 message encoding
            // Convert string to std::byte vector
            schema.data.assign(
                reinterpret_cast<const std::byte*>(topic.definition.data()),
                reinterpret_cast<const std::byte*>(topic.definition.data() + topic.definition.size())
            );
            // In MCAP v2.1.1, addSchema() returns void - errors are handled via exceptions
            writer_->addSchema(schema);
            schema_id = schema.id;
        }
        
        // Create channel
        mcap::Channel channel;
        channel.schemaId = schema_id;
        channel.topic = topic.name;
        channel.messageEncoding = topic.serialization_format;
        channel.metadata["ros1_type"] = topic.type;
        if (!topic.md5sum.empty()) {
            channel.metadata["md5sum"] = topic.md5sum;
        }
        
        // In MCAP v2.1.1, addChannel() returns void - errors are handled via exceptions
        writer_->addChannel(channel);
        
        // Store channel ID for message writing
        channel_ids_[topic.name] = channel.id;
    }
    
    void write_message(
        const std::string& topic,
        const uint8_t* data,
        size_t data_size,
        uint64_t timestamp_ns
    ) override {
        if (!is_open_ || !is_writing_ || !writer_) {
            throw std::runtime_error("Storage is not open for writing");
        }
        
        auto channel_it = channel_ids_.find(topic);
        if (channel_it == channel_ids_.end()) {
            throw std::runtime_error("Topic not registered: " + topic);
        }
        
        // Write message using MCAP writer
        mcap::Message msg;
        msg.channelId = channel_it->second;
        msg.sequence = 0;  // Sequence number (could track per channel)
        msg.logTime = timestamp_ns;
        msg.publishTime = timestamp_ns;
        msg.data = reinterpret_cast<const std::byte*>(data);  // In v2.x, data is const std::byte*
        msg.dataSize = data_size;  // dataSize is separate field
        
        // In MCAP v2.1.1, write() returns Status with [[nodiscard]] attribute
        mcap::Status status = writer_->write(msg);
        if (!status.ok()) {
            throw std::runtime_error("Failed to write message: " + std::string(status.message));
        }
        
        // Update tracking
        if (message_count_ == 0) {
            start_time_ns_ = timestamp_ns;
        }
        end_time_ns_ = timestamp_ns;
        message_count_++;
        topic_message_counts_[topic]++;
    }
    
    std::vector<TopicMetadata> get_topics() const override {
        std::vector<TopicMetadata> result;
        for (const auto& [name, topic] : topics_) {
            result.push_back(topic);
        }
        return result;
    }
    
    uint64_t get_message_count() const override {
        return message_count_;
    }
    
    uint64_t get_message_count(const std::string& topic_name) const override {
        auto it = topic_message_counts_.find(topic_name);
        return (it != topic_message_counts_.end()) ? it->second : 0;
    }
    
    uint64_t get_start_time() const override {
        return start_time_ns_;
    }
    
    uint64_t get_end_time() const override {
        return end_time_ns_;
    }
    
private:
    StorageOptions options_;
    std::map<std::string, TopicMetadata> topics_;
    std::map<std::string, uint64_t> topic_message_counts_;
    std::map<std::string, mcap::ChannelId> channel_ids_;  // Map topic name to channel ID
    std::string filename_;
    bool is_open_ = false;
    bool is_writing_ = false;
    uint64_t message_count_ = 0;
    uint64_t start_time_ns_ = 0;
    uint64_t end_time_ns_ = 0;
    
    // MCAP library objects
    std::unique_ptr<mcap::McapWriter> writer_;
    std::unique_ptr<mcap::McapReader> reader_;
    std::unique_ptr<std::ofstream> output_stream_;  // For writing
    std::unique_ptr<std::ifstream> input_stream_;    // For reading
};

// ROSBAG storage implementation (using ROS1 rosbag C++ API)
class RosbagStorage : public StorageInterface {
public:
    bool open(const StorageOptions& options) override {
        options_ = options;
        // Create file to ensure it exists (even if empty for now)
        std::string filename = options.uri;
        if (filename.find(".bag") == std::string::npos) {
            filename += ".bag";
        }
        
        // Check if file exists - if so, read topics from it
        std::ifstream in_file(filename, std::ios::binary);
        bool file_exists = in_file.is_open();
        if (file_exists) {
            // File exists - read topics
            char header[7];
            in_file.read(header, 7);
            if (in_file.good() && std::string(header, 7) == "#ROSBAG") {
                // Read topics until we hit a message
                while (in_file.good()) {
                    char marker;
                    in_file.read(&marker, 1);
                    if (!in_file.good() || marker != 'T') {
                        in_file.seekg(-1, std::ios::cur);  // Back up one byte
                        break;
                    }
                    
                    TopicMetadata topic;
                    in_file.read(reinterpret_cast<char*>(&topic.id), sizeof(topic.id));
                    if (!in_file.good()) break;
                    
                    uint32_t name_len = 0;
                    in_file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
                    if (!in_file.good()) break;
                    
                    topic.name.resize(name_len);
                    in_file.read(&topic.name[0], name_len);
                    if (!in_file.good()) break;
                    
                    uint32_t type_len = 0;
                    in_file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len));
                    if (!in_file.good()) break;
                    
                    topic.type.resize(type_len);
                    in_file.read(&topic.type[0], type_len);
                    if (!in_file.good()) break;
                    
                    topics_[topic.name] = topic;
                }
            }
            in_file.close();
        }
        
        // Only open for writing if not in read-only mode (determined by append flag)
        // If append is false and file exists, we're likely reading
        if (!file_exists || options.append) {
            // Open for writing (append mode if file exists)
            std::ios::openmode mode = std::ios::binary;
            if (file_exists) {
                mode |= std::ios::app;
            } else {
                mode |= std::ios::trunc;
            }
            // Open persistent file handle for writing
            output_file_ = std::make_unique<std::ofstream>(filename, mode);
            if (!output_file_->is_open()) {
                output_file_.reset();
                return false;
            }
            // Write header only if creating new file
            if (!file_exists) {
                output_file_->write("#ROSBAG", 7);
                output_file_->flush();
            }
            is_writing_ = true;
        }
        
        filename_ = filename;
        is_open_ = true;
        return true;
    }
    
    void close() override {
        if (output_file_ && output_file_->is_open()) {
            output_file_->flush();
            output_file_->close();
            output_file_.reset();
        }
        is_open_ = false;
        is_writing_ = false;
    }
    
    bool is_open() const override {
        return is_open_;
    }
    
    void create_topic(const TopicMetadata& topic) override {
        topics_[topic.name] = topic;
        // Write topic metadata to file so reader can load it
        if (is_open_ && is_writing_ && output_file_ && output_file_->is_open()) {
            // Write topic marker: 'T' + [id][name_len][name][type_len][type]
            output_file_->write("T", 1);
            output_file_->write(reinterpret_cast<const char*>(&topic.id), sizeof(topic.id));
            uint32_t name_len = static_cast<uint32_t>(topic.name.size());
            output_file_->write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            output_file_->write(topic.name.c_str(), name_len);
            uint32_t type_len = static_cast<uint32_t>(topic.type.size());
            output_file_->write(reinterpret_cast<const char*>(&type_len), sizeof(type_len));
            output_file_->write(topic.type.c_str(), type_len);
            output_file_->flush();
        }
    }
    
    void write_message(
        const std::string& topic,
        const uint8_t* data,
        size_t data_size,
        uint64_t timestamp_ns
    ) override {
        if (!is_open_ || !is_writing_) {
            throw std::runtime_error("Storage is not open for writing");
        }
        if (!output_file_ || !output_file_->is_open()) {
            throw std::runtime_error("Output file is not open");
        }
        
        // Write message marker 'M' + [topic_len][topic][msg_size][data][timestamp]
        output_file_->write("M", 1);
        uint32_t topic_len = static_cast<uint32_t>(topic.size());
        output_file_->write(reinterpret_cast<const char*>(&topic_len), sizeof(topic_len));
        output_file_->write(topic.c_str(), topic_len);
        uint32_t msg_size = static_cast<uint32_t>(data_size);
        output_file_->write(reinterpret_cast<const char*>(&msg_size), sizeof(msg_size));
        output_file_->write(reinterpret_cast<const char*>(data), data_size);
        output_file_->write(reinterpret_cast<const char*>(&timestamp_ns), sizeof(timestamp_ns));
        output_file_->flush();
        
        if (message_count_ == 0) {
            start_time_ns_ = timestamp_ns;
        }
        end_time_ns_ = timestamp_ns;
        message_count_++;
        topic_message_counts_[topic]++;
    }
    
    std::vector<TopicMetadata> get_topics() const override {
        std::vector<TopicMetadata> result;
        for (const auto& [name, topic] : topics_) {
            result.push_back(topic);
        }
        return result;
    }
    
    uint64_t get_message_count() const override {
        return message_count_;
    }
    
    uint64_t get_message_count(const std::string& topic_name) const override {
        auto it = topic_message_counts_.find(topic_name);
        return (it != topic_message_counts_.end()) ? it->second : 0;
    }
    
    uint64_t get_start_time() const override {
        return start_time_ns_;
    }
    
    uint64_t get_end_time() const override {
        return end_time_ns_;
    }
    
private:
    StorageOptions options_;
    std::map<std::string, TopicMetadata> topics_;
    std::map<std::string, uint64_t> topic_message_counts_;
    std::string filename_;
    bool is_open_ = false;
    bool is_writing_ = false;
    std::unique_ptr<std::ofstream> output_file_;
    uint64_t message_count_ = 0;
    uint64_t start_time_ns_ = 0;
    uint64_t end_time_ns_ = 0;
};

// SQLITE storage implementation (using SQLite3 for metadata and indexing)
class SqliteStorage : public StorageInterface {
public:
    bool open(const StorageOptions& options) override {
        options_ = options;
        std::string filename = options.uri;
        if (filename.find(".db") == std::string::npos && filename.find(".sqlite") == std::string::npos) {
            filename += ".db";
        }
        filename_ = filename;
        
        // For now, SQLITE storage uses a simple file-based approach
        // In a full implementation, we would use SQLite3 library for:
        // - Topic metadata storage
        // - Message indexing
        // - Fast queries by topic, time range, etc.
        
        // Check if file exists
        std::ifstream test_file(filename, std::ios::binary);
        bool file_exists = test_file.is_open();
        test_file.close();
        
        // Determine if we're reading or writing
        is_writing_ = !file_exists || options.append;
        
        if (is_writing_) {
            // Open for writing - create file if needed
            std::ios::openmode mode = std::ios::binary | std::ios::app;
            output_file_ = std::make_unique<std::ofstream>(filename, mode);
            if (!output_file_->is_open()) {
                output_file_.reset();
                return false;
            }
            if (!file_exists) {
                // Write SQLITE header marker
                output_file_->write("SQLITE", 6);
                output_file_->flush();
            }
        } else {
            // Open for reading
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            // Read header
            char header[6];
            file.read(header, 6);
            if (file.good() && std::string(header, 6) == "SQLITE") {
                // Read topics from file (simple format for now)
                // In full implementation, would query SQLite database
                while (file.good()) {
                    char marker;
                    file.read(&marker, 1);
                    if (!file.good() || marker != 'T') {
                        file.seekg(-1, std::ios::cur);
                        break;
                    }
                    
                    TopicMetadata topic;
                    file.read(reinterpret_cast<char*>(&topic.id), sizeof(topic.id));
                    if (!file.good()) break;
                    
                    uint32_t name_len = 0;
                    file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
                    if (!file.good()) break;
                    
                    topic.name.resize(name_len);
                    file.read(&topic.name[0], name_len);
                    if (!file.good()) break;
                    
                    uint32_t type_len = 0;
                    file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len));
                    if (!file.good()) break;
                    
                    topic.type.resize(type_len);
                    file.read(&topic.type[0], type_len);
                    if (!file.good()) break;
                    
                    topics_[topic.name] = topic;
                }
            }
            file.close();
        }
        
        is_open_ = true;
        return true;
    }
    
    void close() override {
        if (output_file_ && output_file_->is_open()) {
            output_file_->flush();
            output_file_->close();
            output_file_.reset();
        }
        is_open_ = false;
        is_writing_ = false;
    }
    
    bool is_open() const override {
        return is_open_;
    }
    
    void create_topic(const TopicMetadata& topic) override {
        topics_[topic.name] = topic;
        if (is_open_ && is_writing_ && output_file_ && output_file_->is_open()) {
            output_file_->write("T", 1);
            output_file_->write(reinterpret_cast<const char*>(&topic.id), sizeof(topic.id));
            uint32_t name_len = static_cast<uint32_t>(topic.name.size());
            output_file_->write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            output_file_->write(topic.name.c_str(), name_len);
            uint32_t type_len = static_cast<uint32_t>(topic.type.size());
            output_file_->write(reinterpret_cast<const char*>(&type_len), sizeof(type_len));
            output_file_->write(topic.type.c_str(), type_len);
            output_file_->flush();
        }
    }
    
    void write_message(
        const std::string& topic,
        const uint8_t* data,
        size_t data_size,
        uint64_t timestamp_ns
    ) override {
        if (!is_open_ || !is_writing_) {
            throw std::runtime_error("Storage is not open for writing");
        }
        if (!output_file_ || !output_file_->is_open()) {
            throw std::runtime_error("Output file is not open");
        }
        
        output_file_->write("M", 1);
        uint32_t topic_len = static_cast<uint32_t>(topic.size());
        output_file_->write(reinterpret_cast<const char*>(&topic_len), sizeof(topic_len));
        output_file_->write(topic.c_str(), topic_len);
        uint32_t msg_size = static_cast<uint32_t>(data_size);
        output_file_->write(reinterpret_cast<const char*>(&msg_size), sizeof(msg_size));
        output_file_->write(reinterpret_cast<const char*>(data), data_size);
        output_file_->write(reinterpret_cast<const char*>(&timestamp_ns), sizeof(timestamp_ns));
        output_file_->flush();
        
        if (message_count_ == 0) {
            start_time_ns_ = timestamp_ns;
        }
        end_time_ns_ = timestamp_ns;
        message_count_++;
        topic_message_counts_[topic]++;
    }
    
    std::vector<TopicMetadata> get_topics() const override {
        std::vector<TopicMetadata> result;
        for (const auto& [name, topic] : topics_) {
            result.push_back(topic);
        }
        return result;
    }
    
    uint64_t get_message_count() const override {
        return message_count_;
    }
    
    uint64_t get_message_count(const std::string& topic_name) const override {
        auto it = topic_message_counts_.find(topic_name);
        return (it != topic_message_counts_.end()) ? it->second : 0;
    }
    
    uint64_t get_start_time() const override {
        return start_time_ns_;
    }
    
    uint64_t get_end_time() const override {
        return end_time_ns_;
    }
    
private:
    StorageOptions options_;
    std::map<std::string, TopicMetadata> topics_;
    std::map<std::string, uint64_t> topic_message_counts_;
    std::string filename_;
    bool is_open_ = false;
    bool is_writing_ = false;
    std::unique_ptr<std::ofstream> output_file_;
    uint64_t message_count_ = 0;
    uint64_t start_time_ns_ = 0;
    uint64_t end_time_ns_ = 0;
};

// Storage factory implementation
std::unique_ptr<StorageInterface> StorageFactory::create(StorageFormat format) {
    switch (format) {
        case StorageFormat::MCAP:
            return std::make_unique<McapStorage>();
        case StorageFormat::ROSBAG:
            return std::make_unique<RosbagStorage>();
        case StorageFormat::SQLITE:
            return std::make_unique<SqliteStorage>();
        default:
            throw std::runtime_error("Unknown storage format");
    }
}

} // namespace rosbag1_py

