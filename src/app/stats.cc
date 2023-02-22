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
