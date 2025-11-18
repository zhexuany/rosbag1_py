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

#ifndef ROSBAG1_PY_CONSTANTS_HPP
#define ROSBAG1_PY_CONSTANTS_HPP

#include <cstdint>

namespace rosbag1_py {

// Time conversion constants
constexpr uint64_t NANOSECONDS_PER_SECOND = 1000000000ULL;
constexpr double SECONDS_PER_NANOSECOND = 1e-9;

// File size constants
constexpr uint64_t BYTES_PER_KB = 1024ULL;
constexpr uint64_t BYTES_PER_MB = 1024ULL * 1024ULL;
constexpr uint64_t BYTES_PER_GB = 1024ULL * 1024ULL * 1024ULL;

// Default split values
constexpr uint64_t DEFAULT_MAX_SIZE_BYTES = 1ULL * BYTES_PER_GB;  // 1GB
constexpr double DEFAULT_MAX_DURATION_SECONDS = 300.0;  // 5 minutes
constexpr uint64_t DEFAULT_MAX_MESSAGES = 100000ULL;  // 100k messages

} // namespace rosbag1_py

#endif // ROSBAG1_PY_CONSTANTS_HPP

