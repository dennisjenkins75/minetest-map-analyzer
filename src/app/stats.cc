#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>

#include "src/app/stats.h"

void Stats::DumpToFile() {
  std::unique_lock<std::mutex> lock(mutex_);

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
  for (const auto &n : by_type_) {
    ofs << std::setw(12) << n.second << " " << n.first << "\n";
  }
  ofs.close();
}
