#pragma once

#include "src/lib/map_reader/pos.h"
#include <cstdint>

// Keep this struct as SMALL as possible.
// Storing 100M mapblock positions in a std::unordered_map<Pos,Data>
// requires 6.2G of RAM with 16-byte pairs (determined experimentally).  With
// overhead of the container, the average is ~60 bytes per entry (0.65 loading
// factor).
struct MapBlockData {
  MapBlockData() : uniform(0), anthropocene(false), preserve(false) {}

  // If the mapblock is 100% the same content_id, then place that here.
  // 0 otherwise.
  uint16_t uniform;

  // Mapblock contains atleast 1 node that is highly likely to have been placed
  // by a player (and not mapgen).
  bool anthropocene;

  // At least one mapblock within `preserve_radius` is canonically preserved,
  // so this mapblock is as well.
  bool preserve;
};

static_assert(sizeof(MapBlockData) == 4);
