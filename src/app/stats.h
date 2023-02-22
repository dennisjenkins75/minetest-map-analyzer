#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "src/lib/id_map/id_map.h"

// Flush stats every this many blocks.
static constexpr size_t kStatsBlockFlushLimit = 256;

// Used to pass statistica data from consumers to the "StatsQueue", for
// merging into the main stats.  Individual consumer threads have too much
// lock contention to directly manipulate the final stats, so we must use a
// "Map / Reduce" type operation.
struct StatsData {
  StatsData()
      : queued_map_blocks_(0), good_map_blocks_(0), bad_map_blocks_(0) {}

  // Total count of map blocks (set by producer).
  uint64_t queued_map_blocks_;

  // Count of map blocks successfully processed.
  uint64_t good_map_blocks_;

  // Count of map blocks that failed to parse.
  uint64_t bad_map_blocks_;

  void Merge(const StatsData &a);
};

class Stats {
public:
  Stats() : data_(), queue_(), mutex_(), cv_() {}

  uint64_t QueuedBlocks() {
    std::unique_lock<std::mutex> lock(mutex_);
    return data_.good_map_blocks_;
  }

  StatsData Get() const {
    StatsData ret;
    std::unique_lock<std::mutex> lock;
    ret.Merge(data_);
    return ret;
  }

  // Assumes ownership, caller must re-allocate.
  void EnqueueStatsData(std::unique_ptr<StatsData> packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push_back(std::move(packet));
    cv_.notify_one();
  }

  // Queues special node telling `StatsMergeThread()` to stop processing and
  // return.
  void SetTombstone() {
    std::unique_lock<std::mutex> lock(mutex_);
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
