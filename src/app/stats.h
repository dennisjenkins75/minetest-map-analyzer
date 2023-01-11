#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "src/lib/id_map/id_map.h"

// Used to pass statistica data from consumers to the "StatsQueue", for
// merging into the main stats.  Individual consumer threads have too much
// lock contention to directly manipulate the final stats, so we must use a
// "Map / Reduce" type operation.
struct StatsData {
  StatsData()
      : total_map_blocks_(0), bad_map_blocks_(0), by_version_{},
        by_type_(65536, 0) {}

  // Total count of map blocks.
  uint64_t total_map_blocks_;

  // Count of map blocks that failed to parse.
  uint64_t bad_map_blocks_;

  // Count of map blocks for each version.
  // Index is guaranteed to by a uint8_t.
  uint64_t by_version_[256];

  // Count of each node.
  std::vector<uint64_t> by_type_;

  void DumpToFile(const IdMap &node_map);

  void Merge(const StatsData &a);
};

class Stats {
public:
  Stats() : data_(), queue_(), mutex_(), cv_() {}

  void DumpToFile(const IdMap &node_map) {
    std::unique_lock<std::mutex> lock;
    data_.DumpToFile(node_map);
  }

  // Assumes ownership, caller must re-allocate.
  void EnqueueStatsData(std::unique_ptr<StatsData> packet) {
    std::unique_lock<std::mutex> lock;
    queue_.push_back(std::move(packet));
    cv_.notify_one();
  }

  uint64_t TotalBlocks() {
    std::unique_lock<std::mutex> lock;
    return data_.total_map_blocks_;
  }

  // Queues special node telling `StatsMergeThread()` to stop processing and
  // return.
  void SetTombstone() {
    std::unique_lock<std::mutex> lock;
    queue_.push_back(nullptr);
    cv_.notify_all();
  }

  void StatsMergeThread();

private:
  StatsData data_;

  std::deque<std::unique_ptr<StatsData>> queue_;

  mutable std::mutex mutex_;

  std::condition_variable cv_;
};
