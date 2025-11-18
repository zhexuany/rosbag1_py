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

#include "rosbag1_py/compression.hpp"
#include <stdexcept>
#include <map>
#include <functional>
#include <mutex>
#include <cstring>

// Compression library headers
#include <lz4.h>
#include <zstd.h>
#include <bzlib.h>
#include <zlib.h>

namespace rosbag1_py {

// BZ2 compression implementation
class BZ2Compression : public CompressionInterface {
public:
    std::vector<uint8_t> compress(
        const uint8_t* data,
        size_t data_size,
        int level = 9
    ) override {
        if (level < 1 || level > 9) level = 9;
        
        unsigned int dest_len = data_size * 1.01 + 600;
        std::vector<uint8_t> compressed(dest_len);
        
        int result = BZ2_bzBuffToBuffCompress(
            reinterpret_cast<char*>(compressed.data()),
            &dest_len,
            const_cast<char*>(reinterpret_cast<const char*>(data)),
            data_size,
            level,
            0,
            0
        );
        
        if (result != BZ_OK) {
            throw std::runtime_error("BZ2 compression failed");
        }
        
        compressed.resize(dest_len);
        return compressed;
    }
    
    std::vector<uint8_t> decompress(
        const uint8_t* data,
        size_t data_size
    ) override {
        unsigned int dest_len = data_size * 10;  // Estimate
        std::vector<uint8_t> decompressed(dest_len);
        
        int result = BZ2_bzBuffToBuffDecompress(
            reinterpret_cast<char*>(decompressed.data()),
            &dest_len,
            const_cast<char*>(reinterpret_cast<const char*>(data)),
            data_size,
            0,
            0
        );
        
        if (result != BZ_OK) {
            throw std::runtime_error("BZ2 decompression failed");
        }
        
        decompressed.resize(dest_len);
        return decompressed;
    }
    
    std::string get_name() const override {
        return "bz2";
    }
};

// LZ4 compression implementation
class LZ4Compression : public CompressionInterface {
public:
    std::vector<uint8_t> compress(
        const uint8_t* data,
        size_t data_size,
        int /* level */
    ) override {
        int max_compressed_size = LZ4_compressBound(data_size);
        std::vector<uint8_t> compressed(max_compressed_size);
        
        int compressed_size = LZ4_compress_default(
            reinterpret_cast<const char*>(data),
            reinterpret_cast<char*>(compressed.data()),
            data_size,
            max_compressed_size
        );
        
        if (compressed_size <= 0) {
            throw std::runtime_error("LZ4 compression failed");
        }
        
        compressed.resize(compressed_size);
        return compressed;
    }
    
    std::vector<uint8_t> decompress(
        const uint8_t* data,
        size_t data_size
    ) override {
        // LZ4 requires knowing the decompressed size
        // For now, estimate (this should be stored in metadata)
        size_t estimated_size = data_size * 4;
        std::vector<uint8_t> decompressed(estimated_size);
        
        int decompressed_size = LZ4_decompress_safe(
            reinterpret_cast<const char*>(data),
            reinterpret_cast<char*>(decompressed.data()),
            data_size,
            estimated_size
        );
        
        if (decompressed_size < 0) {
            throw std::runtime_error("LZ4 decompression failed");
        }
        
        decompressed.resize(decompressed_size);
        return decompressed;
    }
    
    std::string get_name() const override {
        return "lz4";
    }
};

// ZSTD compression implementation
class ZSTDCompression : public CompressionInterface {
public:
    std::vector<uint8_t> compress(
        const uint8_t* data,
        size_t data_size,
        int level = 3
    ) override {
        if (level < 1 || level > 22) level = 3;
        
        size_t max_compressed_size = ZSTD_compressBound(data_size);
        std::vector<uint8_t> compressed(max_compressed_size);
        
        size_t compressed_size = ZSTD_compress(
            compressed.data(),
            max_compressed_size,
            data,
            data_size,
            level
        );
        
        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error("ZSTD compression failed: " + 
                std::string(ZSTD_getErrorName(compressed_size)));
        }
        
        compressed.resize(compressed_size);
        return compressed;
    }
    
    std::vector<uint8_t> decompress(
        const uint8_t* data,
        size_t data_size
    ) override {
        size_t estimated_size = ZSTD_getFrameContentSize(data, data_size);
        if (estimated_size == ZSTD_CONTENTSIZE_UNKNOWN || 
            estimated_size == ZSTD_CONTENTSIZE_ERROR) {
            estimated_size = data_size * 4;  // Fallback estimate
        }
        
        std::vector<uint8_t> decompressed(estimated_size);
        
        size_t decompressed_size = ZSTD_decompress(
            decompressed.data(),
            estimated_size,
            data,
            data_size
        );
        
        if (ZSTD_isError(decompressed_size)) {
            throw std::runtime_error("ZSTD decompression failed: " + 
                std::string(ZSTD_getErrorName(decompressed_size)));
        }
        
        decompressed.resize(decompressed_size);
        return decompressed;
    }
    
    std::string get_name() const override {
        return "zstd";
    }
};

// GZIP compression implementation
class GZIPCompression : public CompressionInterface {
public:
    std::vector<uint8_t> compress(
        const uint8_t* data,
        size_t data_size,
        int level = 6
    ) override {
        if (level < 1 || level > 9) level = 6;
        
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        
        if (deflateInit2(&stream, level, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            throw std::runtime_error("GZIP initialization failed");
        }
        
        std::vector<uint8_t> compressed(data_size);
        stream.next_in = const_cast<uint8_t*>(data);
        stream.avail_in = data_size;
        stream.next_out = compressed.data();
        stream.avail_out = compressed.size();
        
        int result = deflate(&stream, Z_FINISH);
        deflateEnd(&stream);
        
        if (result != Z_STREAM_END) {
            throw std::runtime_error("GZIP compression failed");
        }
        
        compressed.resize(stream.total_out);
        return compressed;
    }
    
    std::vector<uint8_t> decompress(
        const uint8_t* data,
        size_t data_size
    ) override {
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        
        if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
            throw std::runtime_error("GZIP decompression initialization failed");
        }
        
        std::vector<uint8_t> decompressed(data_size * 4);
        stream.next_in = const_cast<uint8_t*>(data);
        stream.avail_in = data_size;
        stream.next_out = decompressed.data();
        stream.avail_out = decompressed.size();
        
        int result = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        
        if (result != Z_STREAM_END && result != Z_OK) {
            throw std::runtime_error("GZIP decompression failed");
        }
        
        decompressed.resize(stream.total_out);
        return decompressed;
    }
    
    std::string get_name() const override {
        return "gzip";
    }
};

// Compression factory implementation
static std::map<std::string, std::function<std::unique_ptr<CompressionInterface>()>> factories;
static std::once_flag factories_init_flag;
static std::mutex factories_mutex;

static void initialize_factories() {
    // Register default compressors
    factories["bz2"] = []() { return std::make_unique<BZ2Compression>(); };
    factories["lz4"] = []() { return std::make_unique<LZ4Compression>(); };
    factories["zstd"] = []() { return std::make_unique<ZSTDCompression>(); };
    factories["gzip"] = []() { return std::make_unique<GZIPCompression>(); };
}

std::unique_ptr<CompressionInterface> CompressionFactory::create(const std::string& name) {
    std::call_once(factories_init_flag, initialize_factories);
    
    std::lock_guard<std::mutex> lock(factories_mutex);
    auto it = factories.find(name);
    if (it == factories.end()) {
        throw std::runtime_error("Unknown compression type: " + name);
    }
    
    return it->second();
}

bool CompressionFactory::is_available(const std::string& name) {
    std::call_once(factories_init_flag, initialize_factories);
    
    std::lock_guard<std::mutex> lock(factories_mutex);
    return factories.find(name) != factories.end();
}

void CompressionFactory::register_compression(
    const std::string& name,
    std::function<std::unique_ptr<CompressionInterface>()> factory
) {
    std::call_once(factories_init_flag, initialize_factories);
    
    std::lock_guard<std::mutex> lock(factories_mutex);
    factories[name] = factory;
}

} // namespace rosbag1_py

