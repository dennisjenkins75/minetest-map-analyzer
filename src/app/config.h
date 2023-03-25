#pragma once

#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/pos.h"

// User config, captured from command line, shared read-only between worker
// threads.

struct Config {
  Config();

  MapBlockPos min_pos;
  MapBlockPos max_pos;

  // How to read source data (sqlite, postgresql, etc...)
  MapDriverType driver_type;

  // SQLITE: Full path to "map.sqlite" file.
  //   Ex: "${HOME}/.minetest/worlds/myworld/map.sqlite"
  // POSTGRESQL: Connection string.
  //   Ex: "user=minetest password=12345 dbname=myworld port=5432"
  //   Is passed to `PQconnectdb()` unmodified.
  std::string map_filename;

  // Full path to output sqlite file (created by our app, from app/schema).
  std::string out_filename;

  // Full path to input file containing list of node name regexes of nodes
  // to consider "special", in that we treat any mapblock containing any of
  // these nodes as "don't delete" during a map prube operation.
  std::string pattern_filename;

  // Pathname of file to append runtime stats to.
  // Stats are written right before main() exits.
  std::string stats_filename;

  // Count of consumer worker threads.
  // If zero, then the producer and consumer will run serially on the main
  // thread (for easy gdb debugging).
  int threads;

  // max load average.  Worker threads will intentionally slow down if the
  // host's load average is above this value.
  double max_load_avg;

  // Mapblock radius to preserve adjacent anthropocene blocks.
  // The "mapblock removal" code will preserve (not delete) any mapblock within
  // this mapblock distance from any mapblock considered "anthropocene".
  int preserve_radius;

  // Count of MapBlockPos the producer accumulates before locking the common
  // map_block_queue_ to insert them.  Larger values should be make the
  // system more efficient.
  size_t producer_batch_size;

  // Max count of items in each consumer thread's `anthropocene_list` before
  // flushing those to the `preserve_queue_`.
  size_t anthropocene_flush_threshold;

  // How large to let the `PreserveQueue.final_queue_` get, before merging that
  // into the 3d-sparse matrix.
  size_t preserve_limit;

  // If true, track how much "minegeld" (currency) is in each node's metadata.
  // This is expensive (~17% of total CPU usage), so only enable it if needed.
  bool track_minegeld;
};

// Writes config to spdlog.
void DebugLogConfig(const Config &config);
