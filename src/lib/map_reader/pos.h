// pos.h

#ifndef _MT_MAP_SEARCH_POS_H_
#define _MT_MAP_SEARCH_POS_H_

#include <string>

struct Pos {
  int16_t x, y, z;

  Pos() : x(0), y(0), z(0) {}

  // Construct from a "map block pos" (database column), and a "node pos".
  Pos(int64_t mapblock_id, uint16_t node_id);

  // Construct from a node pos.
  Pos(int16_t x, int16_t y, int16_t z);

  // Return mapblock id (assuming pos is a node-pos).
  int64_t mapblock_id_from_node_pos() const;

  std::string str() const;

  static void unit_test();
};

#endif // _MT_MAP_SEARCH_POS_H_
