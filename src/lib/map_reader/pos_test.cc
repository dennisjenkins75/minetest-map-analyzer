#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/map_reader/pos.h"

using ::testing::Eq;
using ::testing::Values;

static inline int64_t block_as_int(int16_t bx, int16_t by, int16_t bz) {
  return (uint64_t)bz * 0x1000000 + (uint64_t)by * 0x1000 + bx;
}

struct PosFromMapBlockPos {
  int64_t mapblock_id; // input
  uint16_t node_id;    // input
  int16_t x;           // expected
  int16_t y;           // expected
  int16_t z;           // expected
};

class PosFromMapBlockPosTest
    : public ::testing::TestWithParam<PosFromMapBlockPos> {};

TEST_P(PosFromMapBlockPosTest, Correct) {
  const auto &param = GetParam();
  const Pos p(param.mapblock_id, param.node_id);
  EXPECT_THAT(p.x, Eq(param.x));
  EXPECT_THAT(p.y, Eq(param.y));
  EXPECT_THAT(p.z, Eq(param.z));
}

INSTANTIATE_TEST_SUITE_P(
    PosFromMapBlockPosTests, PosFromMapBlockPosTest,
    Values(PosFromMapBlockPos{0, 0, 0, 0, 0}, PosFromMapBlockPos{0, 1, 1, 0, 0},
           PosFromMapBlockPos{0, 16, 0, 1, 0},
           PosFromMapBlockPos{0, 256, 0, 0, 1},
           PosFromMapBlockPos{1, 0, 16, 0, 0},
           PosFromMapBlockPos{-1, 0, -16, 0, 0},
           PosFromMapBlockPos{-16781313, 4095, -1, -1, -1},
           PosFromMapBlockPos{-32497465456, 0, 30976, 0, -30992},

           // Lowest block IDs
           PosFromMapBlockPos{block_as_int(-2047, -2047, -2047), 0, -32752,
                              -32752, -32752},
           PosFromMapBlockPos{block_as_int(-2048, -2048, -2048), 0, -32768,
                              -32768, -32768},

           // Highest block id.
           PosFromMapBlockPos{block_as_int(2047, 2047, 2047), 0, 32752, 32752,
                              32752}));

struct MapBlockIdFromCoords {
  int16_t x;           // input
  int16_t y;           // input
  int16_t z;           // input
  int64_t mapblock_id; // expected
};

class MapblockIdFromCoordTest
    : public ::testing::TestWithParam<MapBlockIdFromCoords> {};

TEST_P(MapblockIdFromCoordTest, Correct) {
  const auto &param = GetParam();
  EXPECT_THAT(Pos(param.x, param.y, param.z).mapblock_id_from_node_pos(),
              Eq(param.mapblock_id));
}

INSTANTIATE_TEST_SUITE_P(MapblockIdFromCoordTests, MapblockIdFromCoordTest,
                         Values(MapBlockIdFromCoords{0, 0, 0, 0},
                                MapBlockIdFromCoords{46, 0, -7, -117440466}));
