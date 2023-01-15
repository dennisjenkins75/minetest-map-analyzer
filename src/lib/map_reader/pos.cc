// Convertsion utils copied from LGPL 2.1 minetest code:
// minetest/src/database/database.cpp

#include <iostream>
#include <vector>

#include "src/lib/map_reader/pos.h"

static inline int16_t unsigned_to_signed(uint16_t i, uint16_t max_positive) {
  if (i < max_positive) {
    return i;
  }

  return i - (max_positive * 2);
}

// Modulo of a negative number does not work consistently in C
static inline int64_t pythonmodulo(int64_t i, int16_t mod) {
  if (i >= 0) {
    return i % mod;
  }
  return mod - ((-i) % mod);
}

static inline int64_t block_as_int(int16_t bx, int16_t by, int16_t bz) {
  return (uint64_t)bz * 0x1000000 + (uint64_t)by * 0x1000 + bx;
}

int64_t MapBlockPos::MapBlockId() const { return block_as_int(x, y, z); }

MapBlockPos::MapBlockPos(int64_t mapblock_id) : Pos() {
  x = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - x) / 4096;

  y = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - y) / 4096;

  z = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
}

NodePos::NodePos(int64_t mapblock_id, uint16_t node_id) : Pos() {
  x = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - x) / 4096;

  y = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - y) / 4096;

  z = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);

  x = (x << 4) | (node_id & 15);
  y = (y << 4) | ((node_id >> 4) & 15);
  z = (z << 4) | ((node_id >> 8) & 15);
}

int64_t NodePos::MapBlockId() const {
  return MapBlockPos(x >> 4, y >> 4, z >> 4).MapBlockId();
}
