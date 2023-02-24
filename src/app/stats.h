#pragma once

#include <atomic>
#include <numeric>

// NOTE: Due to `std::atomic` members, this structure is neither copyable nor
// movable.  Accessing multiple members is non-atomic.  However, for our use,
// this is fine.
struct RuntimeStats {
  RuntimeStats()
      : start_time(), flush_time(), end_time(), queued_map_blocks(0),
        good_map_blocks(0), bad_map_blocks(0), peak_vsize_bytes(0) {}

  // Start of the entire process.
  std::chrono::time_point<std::chrono::steady_clock> start_time;

  // When threaded processing is done, and serialized flushing begins.
  std::chrono::time_point<std::chrono::steady_clock> flush_time;

  // End of the entire process.
  std::chrono::time_point<std::chrono::steady_clock> end_time;

  // Total count of map blocks (set by producer).
  std::atomic<uint64_t> queued_map_blocks;

  // Count of map blocks successfully processed.
  std::atomic<uint64_t> good_map_blocks;

  // Count of map blocks that failed to parse.
  std::atomic<uint64_t> bad_map_blocks;

  // Peak VSIZE (bytes).
  std::atomic<size_t> peak_vsize_bytes;

  void SetPeakVSize(size_t current_vsize) {
    // this is not atomic...  sad.
    // TODO: Redo w/ std::atomic<>::compare_exchange_weak().
    peak_vsize_bytes = std::max(peak_vsize_bytes.load(), current_vsize);
  }
};
