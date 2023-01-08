#ifndef _MT_MAP_CONFIG_H
#define _MT_MAP_CONFIG_H

#include <thread>

#include "src/lib/map_reader/pos.h"

// User config, captured from command line, shared read-only between worker
// threads.

struct Config {
  Config()
      : min_pos(MapBlockPos::min()), max_pos(MapBlockPos::max()),
        map_filename(), data_filename(), threads(1),
        max_load_avg(std::thread::hardware_concurrency()) {}

  MapBlockPos min_pos;
  MapBlockPos max_pos;

  // Full path to "map.sqlite" file.
  std::string map_filename;

  // Full path to output sqlite file (created by our app, from app/schema).
  std::string data_filename;

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

#endif // _MT_MAP_CONFIG_H
