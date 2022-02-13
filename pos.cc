// Convertsion utils copied from LGPL 2.1 minetest code:
// minetest/src/database/database.cpp


#include <cassert>
#include <iostream>
#include <vector>

#include "pos.h"
#include "utils.h"

static inline int16_t unsigned_to_signed(uint16_t i, uint16_t max_positive)
{
	if (i < max_positive) {
		return i;
	}

	return i - (max_positive * 2);
}

// Modulo of a negative number does not work consistently in C
static inline int64_t pythonmodulo(int64_t i, int16_t mod)
{
	if (i >= 0) {
		return i % mod;
	}
	return mod - ((-i) % mod);
}

static inline int64_t block_as_int(int16_t bx, int16_t by, int16_t bz) {
  return (uint64_t) bz * 0x1000000 + (uint64_t) by * 0x1000 + bx;
}


Pos::Pos(int16_t x, int16_t y, int16_t z):
  x(x), y(y), z(z) {}

Pos::Pos(int64_t mapblock_id, uint16_t node_id) : x(0), y(0), z(0) {
  x = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - x) / 4096;

  y = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);
  mapblock_id = (mapblock_id - y) / 4096;

  z = unsigned_to_signed(pythonmodulo(mapblock_id, 4096), 2048);

  x = (x << 4) | (node_id & 15);
  y = (y << 4) | ((node_id >> 4) & 15);
  z = (z << 4) | ((node_id >> 8) & 15);
}

int64_t Pos::mapblock_id_from_node_pos() const {
  return block_as_int(x, y, z);
}

std::string Pos::str() const {
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "(%d,%d,%d)", x, y, z);
  return std::string(tmp);
}


void Pos::unit_test() {
  struct TestCase {
    int64_t mapblock_id;
    uint16_t node_id;
    const char *expected;
  };

  const std::vector<TestCase> tests = {
    {0, 0, "(0,0,0)"},
    {0, 1, "(1,0,0)"},
    {0, 16, "(0,1,0)"},
    {0, 256, "(0,0,1)"},
    {1, 0, "(16,0,0)"},
    {-1, 0, "(-16,0,0)"},
    {-16781313, 4095, "(-1,-1,-1)"},
    {-32497465456, 0, "(30976,0,-30992)"},

    // Lowest block ids
    {block_as_int(-2047,-2047,-2047), 0, "(-32752,-32752,-32752)"},
    {block_as_int(-2048,-2048,-2048), 0, "(-32768,-32768,-32768)"},

    // Highest block id.
    {block_as_int(2047,2047,2047), 0, "(32752,32752,32752)"},
  };

  int fail = 0;
  for (auto &test: tests) {
    const Pos p(test.mapblock_id, test.node_id);
    const std::string str = p.str();

    if (test.expected != str) {
      std::cerr << RED << "FAIL " << CLEAR << " expected Pos("
        << GREEN << test.mapblock_id << CLEAR << ", " << GREEN
        << test.node_id << CLEAR << ") = " << GREEN << test.expected
        << CLEAR << ", but got " << RED << str << CLEAR << "\n";
      fail++;
    }
  }

  assert(!fail);
}
