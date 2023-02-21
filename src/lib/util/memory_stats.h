#pragma once

#include <cstdint>

struct MemoryStats {
  MemoryStats() : vsize(0), rss(0) {}

  size_t vsize; // bytes
  size_t rss;   // bytes
};

MemoryStats GetMemoryStats();
