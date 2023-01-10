#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

class Stats {
public:
  Stats()
      : total_map_blocks_(0), bad_map_blocks_(0), by_version_{}, by_type_(),
        mutex_() {}

  void DumpToFile();

  void IncrTotalBlocks() { ++total_map_blocks_; }

  void IncrBadBlocks() { ++bad_map_blocks_; }

  void IncrByVersion(uint8_t version) { ++by_version_[version]; }

  void IncrNodeByType(const std::string &node_name) {
    std::unique_lock<std::mutex> lock(mutex_);
    by_type_[node_name]++;
  }

  int64_t TotalBlocks() { return total_map_blocks_; }

private:
  // Total count of map blocks.
  std::atomic<int64_t> total_map_blocks_;

  // Count of map blocks that failed to parse.
  std::atomic<int64_t> bad_map_blocks_;

  // Count of map blocks for each version.
  std::atomic<int64_t> by_version_[256];

  // Count of nodes of each type.
  std::unordered_map<std::string, int64_t> by_type_;

  std::mutex mutex_;
};
