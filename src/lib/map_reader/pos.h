// pos.h

#ifndef _MT_MAP_SEARCH_POS_H_
#define _MT_MAP_SEARCH_POS_H_

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

template <typename T> struct Pos {
  T x, y, z;

  Pos() : x(0), y(0), z(0) {}

  Pos(const T &x, const T &y, const T &z) : x(x), y(y), z(z) {}

  // Set from a string of the form "x,y,z", where x,y,z are signed integers.
  // Returns 'true' if successful (even if values are out of bounds), and
  // 'false' on failure (input string syntax is invalid).
  // NOTE: format specifier is only valid if template is using int32 or smaller.
  bool parse(const std::string_view &str) {
    int a, b, c;
    if (3 == sscanf(str.data(), "%d,%d,%d", &a, &b, &c)) {
      x = a;
      y = b;
      z = c;
      return true;
    }
    return false;
  }

  // Format as a string.  Ex: "-1734,57,9000"
  std::string str() const {
    std::stringstream ss;
    ss << std::to_string(x) << "," << std::to_string(y) << ","
       << std::to_string(z);
    return ss.str();
  }

  // Returns 'true' if `this` is between `a` and `b` (python rules).
  // Individual dimensions within `a` and `b` are NOT sorted first, caller
  // must ensure that all members of `a` are strictly less than all members of
  // `b`.
  bool inside(const Pos<T> &a, const Pos<T> &b) const {
    return ((a.x <= x) && (a.y <= y) && (a.z <= z) && (x < b.x) && (y < b.y) &&
            (z < b.z));
  }

  // Given another `Pos`, sort their individual dimensions such that `this` has
  // the lesser value from each dimension, and `a` has the greater value.
  void sort(Pos *a) {
    if (x > a->x) {
      std::swap(a->x, x);
    }
    if (y > a->y) {
      std::swap(a->y, y);
    }
    if (z > a->z) {
      std::swap(a->z, z);
    }
  }

  bool operator==(const Pos<T> &a) const {
    return (a.x == x) && (a.y == y) && (a.z == z);
  }
};

// Mapblocks have a range of 12 bits (-2048 to +2047).
struct MapBlockPos : public Pos<int16_t> {
  MapBlockPos() : Pos() {}

  MapBlockPos(int64_t mapblock_id);

  MapBlockPos(int16_t x, int16_t y, int16_t z) : Pos(x, y, z) {}

  // Returns sqlite3 "map.sqlite, blocks.id" value of this mapblock.
  int64_t MapBlockId() const;

  static MapBlockPos min() { return MapBlockPos(-2048, -2048, -2048); }
  static MapBlockPos max() { return MapBlockPos(2047, 2047, 2047); }
};

// Nodes have a range of 16 bits (-32768 to +32767).
struct NodePos : public Pos<int64_t> {
  NodePos() : Pos() {}

  NodePos(int16_t x, int16_t y, int16_t z) : Pos(x, y, z) {}

  // Construct from a "map block pos" (database column), and a relative
  // "node pos" (from within that mapblock).
  NodePos(int64_t mapblock_id, uint16_t node_id);

  // Construct from a "map block pos" (database column), and a relative
  // "node pos" (from within that mapblock).
  NodePos(const MapBlockPos &pos, uint16_t node_id)
      : NodePos(pos.MapBlockId(), node_id) {}

  // Returns sqlite3 "map.sqlite, blocks.id" value of mapblock that contains
  // this node.
  int64_t MapBlockId() const;

  // Returns our own syntheic 48-bit integer.  As far as I know, this number
  // has no meaning inside Minetest.  Its just so that we can use a single int64
  // in our own sqlite database as a key.
  uint64_t NodePosId() const {
    return ((static_cast<uint64_t>(x) & 0xffff) << 0) |
           ((static_cast<uint64_t>(y) & 0xffff) << 16) |
           ((static_cast<uint64_t>(z) & 0xffff) << 32);
  }

  static NodePos min() { return NodePos(-32768, -32768, -32768); }
  static NodePos max() { return NodePos(32767, 32767, 32767); }
};

#endif // _MT_MAP_SEARCH_POS_H_
