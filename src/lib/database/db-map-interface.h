// Abstract interface for interacting with the "map.sqlite" (or postgresql
// equivalent).

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "src/lib/map_reader/pos.h"

enum MapDriverType {
  SQLITE = 0,
  POSTGRESQL = 1,
};

class MapInterface {
public:
  using Blob = std::vector<uint8_t>;

  // Static factory method.
  // Valid driver names are:
  // "sqlite3": sqlite3 backend, connection_string is raw filename.
  // "postgresql" (not implemented yet).
  static std::unique_ptr<MapInterface>
  Create(MapDriverType type, const std::string &connection_str);

  virtual ~MapInterface() {}

  // Returns the raw `map.data` blob, if available, std::nullopt if not.
  virtual std::optional<Blob> LoadMapBlock(const MapBlockPos &pos) = 0;

  // Invoke the callback for each mapblock found between `min` and `max`.
  // Returns `true` after all map blocks are processed AND the `callback`
  // returned true for each map block.  Aborts early on database error and/or
  // if `callback` returns `false`.
  virtual bool
  ProduceMapBlocks(const MapBlockPos &min, const MapBlockPos &max,
                   std::function<bool(const MapBlockPos &pos)> callback) = 0;

  virtual void DeleteMapBlocks(const std::vector<MapBlockPos> &list) = 0;

protected:
  MapInterface() {}
};
