#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <spdlog/spdlog.h>

#include "src/app/stats.h"

void StatsData::Merge(const StatsData &a) {
  queued_map_blocks_ += a.queued_map_blocks_;
  good_map_blocks_ += a.good_map_blocks_;
  bad_map_blocks_ += a.bad_map_blocks_;

  for (size_t i = 0; i < 256; i++) {
    by_version_[i] = a.by_version_[i];
  }

  if (by_type_.size() < a.by_type_.size()) {
    by_type_.resize(a.by_type_.size(), 0);
  }
  for (size_t i = 0; i < a.by_type_.size(); i++) {
    by_type_[i] += a.by_type_[i];
  }
}

void Stats::StatsMergeThread() {
  spdlog::trace("Stats::StatsMergeThread() entry");

  std::unique_lock<std::mutex> lock(mutex_);

  while (true) {
    cv_.wait(lock, [this]() { return !queue_.empty(); });

    // Do we have a tombstone?  If yes, leave it on the queue and exit.
    if (queue_.front() == nullptr) {
      break;
    }

    auto packet = std::move(queue_.front());
    queue_.pop_front();

    data_.Merge(*packet);
  }

  spdlog::trace("Stats::StatsMergeThread() exit");
}
