#include <fstream>
#include <iostream>

#include "src/lib/util/memory_stats.h"

MemoryStats GetMemoryStats() {
  size_t vsize = 0;
  size_t rss = 0;

  std::ifstream ifs("/proc/self/statm");
  if (ifs.is_open()) {
    ifs >> vsize >> rss;
  }

  // TODO: Look up actual page size...
  MemoryStats s;
  s.vsize = vsize * 4096; // pages -> kilobytes.
  s.rss = rss * 4096;     // pages -> kilobytes.

  return s;
}
