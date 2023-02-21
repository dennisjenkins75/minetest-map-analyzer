#pragma once

// Keeps track of mapblocks adajcent to anthropocene mapblocks.
// These mapblocks need to be "preserved" (prevented from deletion).

#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "src/app/config.h"
#include "src/app/mapblock_data.h"
#include "src/lib/3dmatrix/3dmatrix.h"
#include "src/lib/map_reader/pos.h"

class PreserveQueue {
public:
  // Tombstone is an empty set.
  using MapBlockPosSet = std::unordered_set<MapBlockPos, MapBlockPosHashFunc>;

  PreserveQueue() = delete;
  PreserveQueue(const Config &config, Sparse3DMatrix<MapBlockData> &block_data)
      : config_(config), block_data_(block_data), merge_mutex_(), cv_(),
        merge_queue_(), final_mutex_(), final_queue_() {}
  ~PreserveQueue() {}

  void Enqueue(MapBlockPosSet &&pos_set) {
    if (!pos_set.empty()) {
      std::unique_lock<std::mutex> lock(merge_mutex_);
      merge_queue_.push(std::move(pos_set));
      cv_.notify_one();
    }
  }

  void SetTombstone() {
    std::unique_lock<std::mutex> lock(merge_mutex_);
    merge_queue_.push(MapBlockPosSet());
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
  Sparse3DMatrix<MapBlockData> &block_data_;

  std::mutex merge_mutex_;
  std::condition_variable cv_;
  std::queue<MapBlockPosSet> merge_queue_;

  std::mutex final_mutex_;
  MapBlockPosSet final_queue_;

  void DoMerge(MapBlockPosSet &&positions);
};
