#pragma once

// Keeps track of mapblocks adajcent to anthropocene mapblocks.
// These mapblocks need to be "preserved" (prevented from deletion).

#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "src/app/config.h"
#include "src/lib/hashmap/hashmap.h"
#include "src/lib/map_reader/pos.h"

class PreserveQueue {
public:
  using PositionList = std::vector<MapBlockPos>;

  // Tombstone is an empty vector.
  using AnthropoceneQueue = std::queue<PositionList>;

  // Tombstone is an empty set.
  using MapBlockPosSet = std::unordered_set<MapBlockPos, MapBlockPosHashFunc>;

  PreserveQueue() = delete;
  PreserveQueue(const Config &config)
      : config_(config), merge_mutex_(), cv_(), merge_queue_(), final_mutex_(),
        final_queue_() {}
  ~PreserveQueue() {}

  void Enqueue(PositionList &&pos_list) {
    if (!pos_list.empty()) {
      std::unique_lock<std::mutex> lock(merge_mutex_);
      merge_queue_.push(std::move(pos_list));
      cv_.notify_one();
    }
  }

  void SetTombstone() {
    std::unique_lock<std::mutex> lock(merge_mutex_);
    merge_queue_.push(PositionList());
    cv_.notify_all();
  }

  // Consolidates items from `merge_queue_` to `final_queue_`.
  // Exits after tombstone is set.
  void MergeThread();

  // Returns `final_queue_` to the caller, leaving this->final_queue_ empty.
  MapBlockPosSet SurrenderFinalSet() {
    std::unique_lock<std::mutex> lock(final_mutex_);
    MapBlockPosSet ret = std::move(final_queue_);
    final_queue_.clear();
    return ret;
  }

private:
  const Config &config_;

  std::mutex merge_mutex_;
  std::condition_variable cv_;
  AnthropoceneQueue merge_queue_;

  std::mutex final_mutex_;
  MapBlockPosSet final_queue_;

  void DoMerge(PositionList &&positions);
};
