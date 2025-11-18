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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <Python.h>
#include <cstdint>
#include <climits>
#include <cstddef>
#include <iostream>
#include "rosbag1_py/writer.hpp"
#include "rosbag1_py/reader.hpp"
#include "rosbag1_py/storage.hpp"
#include "rosbag1_py/compression.hpp"
#include "rosbag1_py/converter.hpp"
#include "rosbag1_py/splitting.hpp"

namespace py = pybind11;
using namespace rosbag1_py;

PYBIND11_MODULE(rosbag1_py_cpp, m) {
    m.doc() = "rosbag1_py C++ bindings";
    
    // Storage format enum
    py::enum_<StorageFormat>(m, "StorageFormat")
        .value("MCAP", StorageFormat::MCAP)
        .value("ROSBAG", StorageFormat::ROSBAG)
        .value("SQLITE", StorageFormat::SQLITE);
    
    // Storage options
    py::class_<StorageOptions>(m, "StorageOptions")
        .def(py::init<>())
        .def_readwrite("uri", &StorageOptions::uri)
        .def_readwrite("storage_id", &StorageOptions::storage_id)
        .def_readwrite("append", &StorageOptions::append)
        .def_readwrite("custom_data", &StorageOptions::custom_data);
    
    // Topic metadata
    py::class_<TopicMetadata>(m, "TopicMetadata")
        .def(py::init<>())
        .def_readwrite("id", &TopicMetadata::id)
        .def_readwrite("name", &TopicMetadata::name)
        .def_readwrite("type", &TopicMetadata::type)
        .def_readwrite("serialization_format", &TopicMetadata::serialization_format)
        .def_readwrite("md5sum", &TopicMetadata::md5sum)
        .def_readwrite("definition", &TopicMetadata::definition)
        .def_readwrite("metadata", &TopicMetadata::metadata);
    
    // Converter options
    py::class_<ConverterOptions>(m, "ConverterOptions")
        .def(py::init<>())
        .def_readwrite("input_serialization_format", &ConverterOptions::input_serialization_format)
        .def_readwrite("output_serialization_format", &ConverterOptions::output_serialization_format);
    
    // Compression mode enum
    py::enum_<CompressionMode>(m, "CompressionMode")
        .value("NONE", CompressionMode::NONE)
        .value("BZ2", CompressionMode::BZ2)
        .value("LZ4", CompressionMode::LZ4)
        .value("ZSTD", CompressionMode::ZSTD)
        .value("GZIP", CompressionMode::GZIP);
    
    // Compression options
    py::class_<CompressionOptions>(m, "CompressionOptions")
        .def(py::init<>())
        .def_readwrite("compression_mode", &CompressionOptions::compression_mode)
        .def_readwrite("compression_level", &CompressionOptions::compression_level)
        .def_readwrite("compression_queue_size", &CompressionOptions::compression_queue_size)
        .def_readwrite("compression_threads", &CompressionOptions::compression_threads);
    
    // Split mode enum (register before SplitOptions)
    py::enum_<SplitMode>(m, "SplitMode")
        .value("SIZE", SplitMode::SIZE)
        .value("DURATION", SplitMode::DURATION)
        .value("MESSAGE_COUNT", SplitMode::MESSAGE_COUNT);
    
    // Split options (register before Writer::open uses it as default)
    py::class_<SplitOptions>(m, "SplitOptions")
        .def(py::init<>())
        .def_readwrite("mode", &SplitOptions::mode)
        .def_readwrite("max_size", &SplitOptions::max_size)
        .def_readwrite("max_duration", &SplitOptions::max_duration)
        .def_readwrite("max_messages", &SplitOptions::max_messages)
        .def_readwrite("naming_pattern", &SplitOptions::naming_pattern);
    
    // Writer class
    py::class_<Writer>(m, "Writer")
        .def(py::init<>())
        .def("open", [](Writer& self, const StorageOptions& storage_options,
                        const ConverterOptions& converter_options,
                        const CompressionOptions& compression_options,
                        const SplitOptions& split_options) {
            return self.open(storage_options, converter_options, compression_options, split_options);
        }, py::arg("storage_options"),
           py::arg("converter_options") = ConverterOptions(),
           py::arg("compression_options") = CompressionOptions(),
           py::arg("split_options") = SplitOptions())
        .def("close", &Writer::close)
        .def("is_open", &Writer::is_open)
        .def("create_topic", &Writer::create_topic)
        .def("write_message", [](Writer& self, const std::string& topic, py::bytes data, size_t data_size, uint64_t timestamp_ns) {
            // Convert Python bytes to uint8_t*
            std::string data_str = data;
            self.write_message(topic, reinterpret_cast<const uint8_t*>(data_str.data()), data_size, timestamp_ns);
        }, py::arg("topic"),
           py::arg("data"),
           py::arg("data_size"),
           py::arg("timestamp_ns"))
        .def("get_metadata", &Writer::get_metadata);
    
    // Message data
    py::class_<MessageData>(m, "MessageData")
        .def(py::init<>())
        .def_readwrite("topic", &MessageData::topic)
        .def_readwrite("data", &MessageData::data)
        .def_readwrite("timestamp_ns", &MessageData::timestamp_ns)
        .def_readwrite("type", &MessageData::type)
        .def_readwrite("serialization_format", &MessageData::serialization_format);
    
    // Reader class
    py::class_<Reader>(m, "Reader")
        .def(py::init<>())
        .def("open", &Reader::open)
        .def("close", &Reader::close)
        .def("is_open", &Reader::is_open)
        .def("get_topics", &Reader::get_topics)
        // Callback-based read_messages: read_messages_with_callback(callback, topic_filters=None)
        .def("read_messages_with_callback", [](Reader& self, py::function callback, py::object topic_filters_obj) {
            std::vector<std::string> topic_filters;
            if (!topic_filters_obj.is_none()) {
                try {
                    if (py::isinstance<py::list>(topic_filters_obj)) {
                        py::list filters_list = topic_filters_obj.cast<py::list>();
                        for (auto item : filters_list) {
                            topic_filters.push_back(item.cast<std::string>());
                        }
                    } else {
                        topic_filters = topic_filters_obj.cast<std::vector<std::string>>();
                    }
                } catch (...) {
                    // Ignore cast errors
                }
            }
            MessageCallback cpp_callback = [callback](const MessageData& msg) {
                py::gil_scoped_acquire acquire;  // Acquire GIL before calling Python
                callback(msg);
            };
            self.read_messages(cpp_callback, topic_filters);
        }, py::arg("callback"), py::arg("topic_filters") = py::none())
        // Iterator-based read_messages: read_messages(topic_filters=[], start_time_ns=0, end_time_ns=UINT64_MAX)
        .def("read_messages", py::overload_cast<const std::vector<std::string>&, uint64_t, uint64_t>(
            &Reader::read_messages),
            py::arg("topic_filters") = std::vector<std::string>(),
            py::arg("start_time_ns") = static_cast<uint64_t>(0),
            py::arg("end_time_ns") = UINT64_MAX)
        .def("get_metadata", &Reader::get_metadata);
    
    // Bag metadata
    py::class_<Reader::BagMetadata>(m, "BagMetadata")
        .def(py::init<>())
        .def_readwrite("message_count", &Reader::BagMetadata::message_count)
        .def_readwrite("start_time_ns", &Reader::BagMetadata::start_time_ns)
        .def_readwrite("end_time_ns", &Reader::BagMetadata::end_time_ns)
        .def_readwrite("topics_with_message_count", &Reader::BagMetadata::topics_with_message_count);
    
    // Bag splitter
    py::class_<BagSplitter>(m, "BagSplitter")
        .def(py::init<const SplitOptions&>())
        .def("should_split", &BagSplitter::should_split)
        .def("get_next_filename", &BagSplitter::get_next_filename)
        .def("reset_counters", &BagSplitter::reset_counters)
        .def("update_counters", &BagSplitter::update_counters)
        .def("get_current_index", &BagSplitter::get_current_index);
    
    // Type converter
    py::class_<TypeConverter>(m, "TypeConverter")
        .def_static("ros1_to_ros2", &TypeConverter::ros1_to_ros2)
        .def_static("ros2_to_ros1", &TypeConverter::ros2_to_ros1)
        .def_static("is_ros1_type", &TypeConverter::is_ros1_type)
        .def_static("is_ros2_type", &TypeConverter::is_ros2_type);
    
    // Serialization converter
    py::class_<SerializationConverter>(m, "SerializationConverter")
        .def_static("ros1_to_cdr", &SerializationConverter::ros1_to_cdr)
        .def_static("cdr_to_ros1", &SerializationConverter::cdr_to_ros1)
        .def_static("add_cdr_header", &SerializationConverter::add_cdr_header)
        .def_static("remove_cdr_header", &SerializationConverter::remove_cdr_header);
    
    // Time converter
    py::class_<TimeConverter>(m, "TimeConverter")
        .def_static("ros1_to_nanoseconds", &TimeConverter::ros1_to_nanoseconds)
        .def_static("nanoseconds_to_ros1", &TimeConverter::nanoseconds_to_ros1);
}

