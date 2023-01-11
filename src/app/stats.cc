#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <spdlog/spdlog.h>

#include "src/app/stats.h"

void StatsData::DumpToFile(const IdMap &node_map) {
  std::ofstream ofs("stats.out");

  ofs << "bad_map_blocks: " << bad_map_blocks_ << "\n";
  ofs << "total_map_blocks: " << total_map_blocks_ << "\n";
  ofs << "bad %: "
      << 100.0 * static_cast<double>(bad_map_blocks_) /
             static_cast<double>(total_map_blocks_)
      << "\n";

  for (int i = 0; i < 256; i++) {
    if (by_version_[i]) {
      ofs << "version: " << i << " = " << by_version_[i] << "\n";
    }
  }
  ofs.close();

  ofs.open("nodes-by-type.out");
  for (size_t idx = 0; idx < std::min(by_type_.size(), node_map.size());
       idx++) {
    ofs << node_map.Get(idx) << " " << by_type_.at(idx) << "\n";
  }
  ofs.close();
}

void StatsData::Merge(const StatsData &a) {
  total_map_blocks_ += a.total_map_blocks_;
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
