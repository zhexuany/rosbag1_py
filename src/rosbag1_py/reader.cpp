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

#include "rosbag1_py/reader.hpp"
#include "rosbag1_py/storage.hpp"
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <map>
#include <unordered_set>
#include <cstddef>
#include <iostream>
#include <mcap/reader.hpp>
#include <mcap/types.hpp>

namespace rosbag1_py {

Reader::Reader() : is_open_(false) {
}

Reader::~Reader() {
    if (is_open_) {
        close();
    }
}

void Reader::open(const StorageOptions& storage_options) {
    if (is_open_) {
        throw std::runtime_error("Reader is already open");
    }
    
    storage_options_ = storage_options;
    // Set append to false for read-only mode
    storage_options_.append = false;
    
    // Create storage backend
    storage_ = StorageFactory::create(storage_options_.storage_id);
    
    if (!storage_->open(storage_options_)) {
        throw std::runtime_error("Failed to open storage");
    }
    
    is_open_ = true;
}

void Reader::close() {
    if (!is_open_) {
        return;
    }
    
    if (storage_) {
        storage_->close();
        storage_.reset();
    }
    
    is_open_ = false;
}

bool Reader::is_open() const {
    return is_open_;
}

std::vector<TopicMetadata> Reader::get_topics() const {
    if (!is_open_) {
        throw std::runtime_error("Reader is not open");
    }
    
    return storage_->get_topics();
}

void Reader::read_messages(
    const MessageCallback& callback,
    const std::vector<std::string>& topic_filters
) {
    if (!is_open_) {
        throw std::runtime_error("Reader is not open");
    }
    
    // Convert topic filters to unordered_set for O(1) lookup
    std::unordered_set<std::string> topic_filter_set(topic_filters.begin(), topic_filters.end());
    
    // Get all topics to map topic names
    auto topics = storage_->get_topics();
    std::map<std::string, TopicMetadata> topic_map;
    for (const auto& topic : topics) {
        topic_map[topic.name] = topic;
    }
    
    // Read messages from file and call callback for each
    std::string filename = storage_options_.uri;
    // Add extension if not present (storage adds it when creating file)
    if (storage_options_.storage_id == StorageFormat::MCAP && filename.find(".mcap") == std::string::npos) {
        filename += ".mcap";
    } else if (storage_options_.storage_id == StorageFormat::ROSBAG && filename.find(".bag") == std::string::npos) {
        filename += ".bag";
    }
    
    if (filename.find(".mcap") != std::string::npos) {
        // Use MCAP reader for callback-based reading
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open MCAP file for reading: " + filename);
        }
        
        mcap::McapReader reader;
        auto status = reader.open(file);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open MCAP reader: " + std::string(status.message));
        }
        
        // Build channel ID to topic name
        // Channels may be in the summary (read by open()) or discovered by reading messages
        std::map<mcap::ChannelId, std::string> channel_to_topic;
        
        // First, try to get channels from summary (if available)
        for (const auto& [channel_id, channel] : reader.channels()) {
            channel_to_topic[channel_id] = channel->topic;
        }
        
        // If no channels found in summary, we need to discover them by reading messages first
        if (channel_to_topic.empty()) {
            // IMPORTANT: Don't look up channels during reading - MCAP may return incorrect channels
            // Wait until after all messages are read, then build the map from reader.channels()
            auto discovery_view = reader.readMessages();
            for (const auto& msg_view : discovery_view) {
                (void)msg_view;  // Just iterate to discover channels, don't look them up yet
            }
            // After reading all messages, build the channel map from reader.channels()
            // This ensures we get the correct channel-to-topic mappings
            for (const auto& [channel_id, channel] : reader.channels()) {
                channel_to_topic[channel_id] = channel->topic;
            }
            // Close and reopen to reset position for actual reading
            reader.close();
            file.close();
            file = std::ifstream(filename, std::ios::binary);
            if (!file.is_open()) {
                file.close();
                reader.close();
                throw std::runtime_error("Failed to reopen MCAP file for reading: " + filename);
            }
            status = reader.open(file);
            if (!status.ok()) {
                file.close();
                reader.close();
                throw std::runtime_error("Failed to reopen MCAP reader: " + std::string(status.message));
            }
            // Update map with any channels that are now available from summary
            for (const auto& [channel_id, channel] : reader.channels()) {
                channel_to_topic[channel_id] = channel->topic;
            }
            
            // If map is still empty after reopening (no summary), we need to discover channels again
            if (channel_to_topic.empty()) {
                auto discovery_view = reader.readMessages();
                for (const auto& msg_view : discovery_view) {
                    (void)msg_view;  // Just iterate to discover channels
                }
                // Build map from discovered channels
                for (const auto& [channel_id, channel] : reader.channels()) {
                    channel_to_topic[channel_id] = channel->topic;
                }
                // Close and reopen again to reset position for actual reading
                reader.close();
                file.close();
                file = std::ifstream(filename, std::ios::binary);
                if (!file.is_open()) {
                    file.close();
                    reader.close();
                    throw std::runtime_error("Failed to reopen MCAP file for reading: " + filename);
                }
                status = reader.open(file);
                if (!status.ok()) {
                    file.close();
                    reader.close();
                    throw std::runtime_error("Failed to reopen MCAP reader: " + std::string(status.message));
                }
                // Update map with channels from summary (if available now)
                for (const auto& [channel_id, channel] : reader.channels()) {
                    channel_to_topic[channel_id] = channel->topic;
                }
            }
        }
        
        // Read messages using MCAP reader and call callback
        auto message_view = reader.readMessages();
        size_t total_messages = 0;
        size_t filtered_messages = 0;
        size_t callback_called = 0;
        
        for (const auto& msg_view : message_view) {
            total_messages++;
            // In MCAP v2.x, MessageView is the iterator result
            const mcap::MessageView& view = msg_view;
            const mcap::Message& mcap_msg = view.message;
            
            // Get topic name from channel ID
            // If channel not in map yet, try to get it from reader (discovered during read)
            auto channel_it = channel_to_topic.find(mcap_msg.channelId);
            if (channel_it == channel_to_topic.end()) {
                // Channel might have been discovered during message reading
                auto discovered_channel_it = reader.channels().find(mcap_msg.channelId);
                if (discovered_channel_it != reader.channels().end()) {
                    std::string topic_name = discovered_channel_it->second->topic;
                    channel_to_topic[mcap_msg.channelId] = topic_name;
                    channel_it = channel_to_topic.find(mcap_msg.channelId);
                }
            }
            if (channel_it == channel_to_topic.end()) {
                continue;  // Skip if channel still not found
            }
            std::string topic_name = channel_it->second;
            
            // Apply topic filter (O(1) lookup)
            if (!topic_filter_set.empty()) {
                if (topic_filter_set.find(topic_name) == topic_filter_set.end()) {
                    filtered_messages++;
                    continue;
                }
            }
            
            // Convert MCAP message to MessageData
            MessageData msg;
            msg.topic = topic_name;
            msg.timestamp_ns = mcap_msg.logTime;
            
            // In MCAP v2.x, data is const std::byte* and dataSize is separate
            msg.data.resize(mcap_msg.dataSize);
            // Convert std::byte* to uint8_t*
            std::transform(mcap_msg.data, mcap_msg.data + mcap_msg.dataSize, 
                         msg.data.begin(), 
                         [](std::byte b) { return static_cast<uint8_t>(b); });
            
            // Get message type from topic map
            if (topic_map.find(topic_name) != topic_map.end()) {
                msg.type = topic_map[topic_name].type;
                msg.serialization_format = topic_map[topic_name].serialization_format;
            }
            
            try {
                callback_called++;
                callback(msg);
            } catch (...) {
                // If callback throws, we continue reading
            }
        }
        
        reader.close();
        file.close();
    } else if (filename.find(".db") != std::string::npos || filename.find(".sqlite") != std::string::npos) {
        // SQLITE format - use simple format reading
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.seekg(6);  // Skip "SQLITE" header
            
            size_t total_messages = 0;
            size_t filtered_messages = 0;
            size_t callback_called = 0;
            
            // Read through file, skipping topics and calling callback for messages
            char marker;
            while (true) {
                file.read(&marker, 1);
                if (file.eof() || file.fail()) {
                    break;
                }
                
                if (marker == 'T') {
                    // Skip topic metadata
                    uint32_t id;
                    if (!file.read(reinterpret_cast<char*>(&id), sizeof(id))) break;
                    uint32_t name_len;
                    if (!file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len))) break;
                    file.seekg(name_len, std::ios::cur);
                    uint32_t type_len;
                    if (!file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len))) break;
                    file.seekg(type_len, std::ios::cur);
                    continue;
                } else if (marker == 'M') {
                    total_messages++;
                    // Read message
                    uint32_t topic_len = 0;
                    file.read(reinterpret_cast<char*>(&topic_len), sizeof(topic_len));
                    if (file.eof() || file.fail() || topic_len == 0) break;
                    
                    std::string topic_name(topic_len, '\0');
                    file.read(&topic_name[0], topic_len);
                    if (file.eof() || file.fail()) break;
                    
                    // Apply topic filter (O(1) lookup)
                    if (!topic_filter_set.empty()) {
                        if (topic_filter_set.find(topic_name) == topic_filter_set.end()) {
                            filtered_messages++;
                            uint32_t msg_size = 0;
                            file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                            if (file.eof() || file.fail()) break;
                            file.seekg(msg_size + sizeof(uint64_t), std::ios::cur);
                            continue;
                        }
                    }
                    
                    uint32_t msg_size = 0;
                    file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                    if (file.eof() || file.fail() || msg_size == 0) break;
                    
                    std::vector<uint8_t> data(msg_size);
                    file.read(reinterpret_cast<char*>(data.data()), msg_size);
                    if (file.eof() || file.fail()) break;
                    
                    uint64_t timestamp_ns = 0;
                    file.read(reinterpret_cast<char*>(&timestamp_ns), sizeof(timestamp_ns));
                    if (file.eof() || file.fail()) break;
                    
                    MessageData msg;
                    msg.topic = topic_name;
                    msg.data = data;
                    msg.timestamp_ns = timestamp_ns;
                    if (topic_map.find(topic_name) != topic_map.end()) {
                        msg.type = topic_map[topic_name].type;
                        msg.serialization_format = topic_map[topic_name].serialization_format;
                    }
                    
                    try {
                        callback_called++;
                        callback(msg);
                    } catch (...) {
                        // Continue on callback exception
                    }
                } else {
                    break;
                }
            }
            
            file.close();
        }
    } else if (filename.find(".bag") != std::string::npos) {
        // ROSBAG format - use simple format reading
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.seekg(7);  // Skip "#ROSBAG" header
            
            size_t total_messages = 0;
            size_t filtered_messages = 0;
            size_t callback_called = 0;
            
            // Read through file, skipping topics and calling callback for messages
            char marker;
            while (true) {
                file.read(&marker, 1);
                if (file.eof() || file.fail()) {
                    break;
                }
                
                if (marker == 'T') {
                    // Skip topic metadata
                    uint32_t id;
                    if (!file.read(reinterpret_cast<char*>(&id), sizeof(id))) break;
                    uint32_t name_len;
                    if (!file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len))) break;
                    file.seekg(name_len, std::ios::cur);
                    uint32_t type_len;
                    if (!file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len))) break;
                    file.seekg(type_len, std::ios::cur);
                    continue;
                } else if (marker == 'M') {
                    total_messages++;
                    // Read message
                    uint32_t topic_len = 0;
                    file.read(reinterpret_cast<char*>(&topic_len), sizeof(topic_len));
                    if (file.eof() || file.fail() || topic_len == 0) break;
                    
                    std::string topic_name(topic_len, '\0');
                    file.read(&topic_name[0], topic_len);
                    if (file.eof() || file.fail()) break;
                    
                    // Apply topic filter (O(1) lookup)
                    if (!topic_filter_set.empty()) {
                        if (topic_filter_set.find(topic_name) == topic_filter_set.end()) {
                            filtered_messages++;
                            uint32_t msg_size = 0;
                            file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                            if (file.eof() || file.fail()) break;
                            file.seekg(msg_size + sizeof(uint64_t), std::ios::cur);
                            continue;
                        }
                    }
                    
                    uint32_t msg_size = 0;
                    file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                    if (file.eof() || file.fail() || msg_size == 0) break;
                    
                    std::vector<uint8_t> data(msg_size);
                    file.read(reinterpret_cast<char*>(data.data()), msg_size);
                    if (file.eof() || file.fail()) break;
                    
                    uint64_t timestamp_ns = 0;
                    file.read(reinterpret_cast<char*>(&timestamp_ns), sizeof(timestamp_ns));
                    if (file.eof() || file.fail()) break;
                    
                    MessageData msg;
                    msg.topic = topic_name;
                    msg.data = data;
                    msg.timestamp_ns = timestamp_ns;
                    if (topic_map.find(topic_name) != topic_map.end()) {
                        msg.type = topic_map[topic_name].type;
                        msg.serialization_format = topic_map[topic_name].serialization_format;
                    }
                    
                    try {
                        callback_called++;
                        callback(msg);
                    } catch (...) {
                        // Continue on callback exception
                    }
                } else {
                    break;
                }
            }
            
            file.close();
        }
    }
}

std::vector<MessageData> Reader::read_messages(
    const std::vector<std::string>& topic_filters,
    uint64_t start_time_ns,
    uint64_t end_time_ns
) {
    if (!is_open_) {
        throw std::runtime_error("Reader is not open");
    }
    
    std::vector<MessageData> messages;
    
    // Convert topic filters to unordered_set for O(1) lookup
    std::unordered_set<std::string> topic_filter_set(topic_filters.begin(), topic_filters.end());
    
    // Get all topics to map topic names
    auto topics = storage_->get_topics();
    std::map<std::string, TopicMetadata> topic_map;
    for (const auto& topic : topics) {
        topic_map[topic.name] = topic;
    }
    
    // Read messages from file
    std::string filename = storage_options_.uri;
    // Add extension if not present (storage adds it when creating file)
    if (storage_options_.storage_id == StorageFormat::MCAP && filename.find(".mcap") == std::string::npos) {
        filename += ".mcap";
    } else if (storage_options_.storage_id == StorageFormat::ROSBAG && filename.find(".bag") == std::string::npos) {
        filename += ".bag";
    } else if (storage_options_.storage_id == StorageFormat::SQLITE && 
               filename.find(".db") == std::string::npos && filename.find(".sqlite") == std::string::npos) {
        filename += ".db";
    }
    
    if (filename.find(".mcap") != std::string::npos) {
        // Use MCAP reader to read messages
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            mcap::McapReader reader;
            auto status = reader.open(file);
            if (status.ok()) {
                // Build channel ID to topic name map
                // Channels may be in the summary (read by open()) or discovered by reading messages
                std::map<mcap::ChannelId, std::string> channel_to_topic;
                
                // First, try to get channels from summary (if available)
                for (const auto& [channel_id, channel] : reader.channels()) {
                    channel_to_topic[channel_id] = channel->topic;
                }
                
                // If no channels found in summary, we need to discover them by reading messages first
                if (channel_to_topic.empty()) {
                    // IMPORTANT: Don't look up channels during reading - MCAP may return incorrect channels
                    // Wait until after all messages are read, then build the map from reader.channels()
                    auto discovery_view = reader.readMessages();
                    for (const auto& msg_view : discovery_view) {
                        (void)msg_view;  // Just iterate to discover channels, don't look them up yet
                    }
                    // After reading all messages, build the channel map from reader.channels()
                    // This ensures we get the correct channel-to-topic mappings
                    for (const auto& [channel_id, channel] : reader.channels()) {
                        channel_to_topic[channel_id] = channel->topic;
                    }
                    // Close and reopen to reset position for actual reading
                    reader.close();
                    file.close();
                    file = std::ifstream(filename, std::ios::binary);
                    if (!file.is_open()) {
                        // Can't continue if file can't be reopened, return empty vector
                        return messages;
                    }
                    status = reader.open(file);
                    if (!status.ok()) {
                        // Can't continue if reader can't be reopened, return empty vector
                        file.close();
                        return messages;
                    }
                    // Update map with any channels that are now available from summary
                    for (const auto& [channel_id, channel] : reader.channels()) {
                        channel_to_topic[channel_id] = channel->topic;
                    }
                }
                
                // Read messages using MCAP reader
                auto message_view = reader.readMessages();
                for (const auto& msg_view : message_view) {
                    // In MCAP v2.x, MessageView is the iterator result
                    const mcap::MessageView& view = msg_view;
                    const mcap::Message& mcap_msg = view.message;
                    
                    // Get topic name from channel ID
                    // If channel not in map yet, try to get it from reader (discovered during read)
                    auto channel_it = channel_to_topic.find(mcap_msg.channelId);
                    if (channel_it == channel_to_topic.end()) {
                        // Channel might have been discovered during message reading
                        auto discovered_channel_it = reader.channels().find(mcap_msg.channelId);
                        if (discovered_channel_it != reader.channels().end()) {
                            std::string topic_name = discovered_channel_it->second->topic;
                            channel_to_topic[mcap_msg.channelId] = topic_name;
                            channel_it = channel_to_topic.find(mcap_msg.channelId);
                        }
                    }
                    if (channel_it == channel_to_topic.end()) {
                        continue;  // Skip if channel still not found
                    }
                    std::string topic_name = channel_it->second;
                    
                    // Apply topic filter (O(1) lookup)
                    if (!topic_filter_set.empty()) {
                        if (topic_filter_set.find(topic_name) == topic_filter_set.end()) {
                            continue;
                        }
                    }
                    
                    // Apply time filter
                    uint64_t timestamp_ns = mcap_msg.logTime;
                    if (timestamp_ns < start_time_ns || timestamp_ns > end_time_ns) {
                        continue;
                    }
                    
                    // Convert MCAP message to MessageData
                    MessageData msg;
                    msg.topic = topic_name;
                    msg.timestamp_ns = timestamp_ns;
                    
                    // In MCAP v2.x, data is const std::byte* and dataSize is separate
                    msg.data.resize(mcap_msg.dataSize);
                    // Convert std::byte* to uint8_t*
                    std::transform(mcap_msg.data, mcap_msg.data + mcap_msg.dataSize, 
                                 msg.data.begin(), 
                                 [](std::byte b) { return static_cast<uint8_t>(b); });
                    
                    // Get message type from topic map
                    if (topic_map.find(topic_name) != topic_map.end()) {
                        msg.type = topic_map[topic_name].type;
                        msg.serialization_format = topic_map[topic_name].serialization_format;
                    }
                    
                    messages.push_back(msg);
                }
                reader.close();
            }
            file.close();
        }
    } else if (filename.find(".bag") != std::string::npos) {
        // Skip ROSBAG header (7 bytes)
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.seekg(7);  // Skip "#ROSBAG" header
            
            // Read through file, skipping topics and collecting messages
            char marker;
            while (true) {
                // Read next marker
                file.read(&marker, 1);
                if (file.eof() || file.fail()) {
                    // End of file or error
                    break;
                }
                
                if (marker == 'T') {
                    // Skip topic metadata: [id][name_len][name][type_len][type]
                    uint32_t id;
                    if (!file.read(reinterpret_cast<char*>(&id), sizeof(id))) break;
                    
                    uint32_t name_len;
                    if (!file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len))) break;
                    
                    // Skip name bytes
                    file.seekg(name_len, std::ios::cur);
                    
                    uint32_t type_len;
                    if (!file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len))) break;
                    
                    // Skip type bytes
                    file.seekg(type_len, std::ios::cur);
                    
                    // Continue to next marker
                    continue;
                } else if (marker == 'M') {
                    // Found a message marker - read it
                    // Read topic name
                    uint32_t topic_len = 0;
                    file.read(reinterpret_cast<char*>(&topic_len), sizeof(topic_len));
                    if (file.eof() || file.fail() || topic_len == 0) break;
                    
                    std::string topic_name(topic_len, '\0');
                    file.read(&topic_name[0], topic_len);
                    if (file.eof() || file.fail()) break;
                    
                    // Apply topic filter (O(1) lookup)
                    if (!topic_filter_set.empty()) {
                        if (topic_filter_set.find(topic_name) == topic_filter_set.end()) {
                            // Skip this message - read past it
                            uint32_t msg_size = 0;
                            file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                            if (file.eof() || file.fail()) break;
                            file.seekg(msg_size + sizeof(uint64_t), std::ios::cur);
                            if (file.eof() || file.fail()) {
                                if (file.eof()) break;
                                file.clear();
                            }
                            continue;
                        }
                    }
                    
                    // Read message data
                    uint32_t msg_size = 0;
                    file.read(reinterpret_cast<char*>(&msg_size), sizeof(msg_size));
                    if (file.eof() || file.fail() || msg_size == 0) break;
                    
                    std::vector<uint8_t> data(msg_size);
                    file.read(reinterpret_cast<char*>(data.data()), msg_size);
                    if (file.eof() || file.fail()) break;
                    
                    uint64_t timestamp_ns = 0;
                    file.read(reinterpret_cast<char*>(&timestamp_ns), sizeof(timestamp_ns));
                    if (file.eof() || file.fail()) break;
                    
                    // Apply time filter
                    if (timestamp_ns < start_time_ns || timestamp_ns > end_time_ns) {
                        continue;
                    }
                    
                    MessageData msg;
                    msg.topic = topic_name;
                    msg.data = data;
                    msg.timestamp_ns = timestamp_ns;
                    if (topic_map.find(topic_name) != topic_map.end()) {
                        msg.type = topic_map[topic_name].type;
                        msg.serialization_format = topic_map[topic_name].serialization_format;
                    }
                    messages.push_back(msg);
                } else {
                    // Unknown marker, skip it or break
                    break;
                }
            }
            file.close();
        }
    }
    
    return messages;
}

Reader::BagMetadata Reader::get_metadata() const {
    if (!is_open_) {
        throw std::runtime_error("Reader is not open");
    }
    
    BagMetadata metadata;
    metadata.message_count = storage_->get_message_count();
    metadata.start_time_ns = storage_->get_start_time();
    metadata.end_time_ns = storage_->get_end_time();
    
    // Get topics with message counts
    auto topics = storage_->get_topics();
    for (const auto& topic : topics) {
        uint64_t count = storage_->get_message_count(topic.name);
        metadata.topics_with_message_count.push_back(
            std::make_tuple(topic.name, topic.type, count)
        );
    }
    
    return metadata;
}

} // namespace rosbag1_py

