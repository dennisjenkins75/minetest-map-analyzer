#pragma once

#include <thread>

#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/pos.h"

// User config, captured from command line, shared read-only between worker
// threads.

struct Config {
  Config()
      : min_pos(MapBlockPos::min()), max_pos(MapBlockPos::max()),
        driver_type(MapDriverType::SQLITE), map_filename(), out_filename(),
        pattern_filename(), threads(0),
        max_load_avg(std::thread::hardware_concurrency()) {}

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

  // Count of consumer worker threads.
  // If zero, then the producer and consumer will run serially on the main
  // thread (for easy gdb debugging).
  int threads;

  // max load average.  Worker threads will intentionally slow down if the
  // host's load average is above this value.
  double max_load_avg;
};

// Writes config to spdlog.
void DebugLogConfig(const Config &config);
