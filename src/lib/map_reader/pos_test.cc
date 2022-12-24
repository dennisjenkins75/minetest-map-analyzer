#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/map_reader/pos.h"

using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Values;

static inline int64_t block_as_int(int16_t bx, int16_t by, int16_t bz) {
  return (uint64_t)bz * 0x1000000 + (uint64_t)by * 0x1000 + bx;
}

struct NodePosFromMapBlockPos {
  int64_t mapblock_id; // input
  uint16_t node_id;    // input
  int16_t x;           // expected
  int16_t y;           // expected
  int16_t z;           // expected
};

class NodePosFromMapBlockPosTest
    : public ::testing::TestWithParam<NodePosFromMapBlockPos> {};

TEST_P(NodePosFromMapBlockPosTest, Correct) {
  const auto &param = GetParam();
  const NodePos p(param.mapblock_id, param.node_id);
  EXPECT_THAT(p.x, Eq(param.x));
  EXPECT_THAT(p.y, Eq(param.y));
  EXPECT_THAT(p.z, Eq(param.z));
}

INSTANTIATE_TEST_SUITE_P(
    NodePosFromMapBlockPosTests, NodePosFromMapBlockPosTest,
    Values(NodePosFromMapBlockPos{0, 0, 0, 0, 0},
           NodePosFromMapBlockPos{0, 1, 1, 0, 0},
           NodePosFromMapBlockPos{0, 16, 0, 1, 0},
           NodePosFromMapBlockPos{0, 256, 0, 0, 1},
           NodePosFromMapBlockPos{1, 0, 16, 0, 0},
           NodePosFromMapBlockPos{-1, 0, -16, 0, 0},
           NodePosFromMapBlockPos{-16781313, 4095, -1, -1, -1},
           NodePosFromMapBlockPos{-32497465456, 0, 30976, 0, -30992},

           // Lowest block IDs
           NodePosFromMapBlockPos{block_as_int(-2047, -2047, -2047), 0, -32752,
                                  -32752, -32752},
           NodePosFromMapBlockPos{block_as_int(-2048, -2048, -2048), 0, -32768,
                                  -32768, -32768},

           // Highest block id.
           NodePosFromMapBlockPos{block_as_int(2047, 2047, 2047), 0, 32752,
                                  32752, 32752}));

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
  EXPECT_THAT(MapBlockPos(param.x, param.y, param.z).MapBlockId(),
              Eq(param.mapblock_id));
}

INSTANTIATE_TEST_SUITE_P(MapblockIdFromCoordTests, MapblockIdFromCoordTest,
                         Values(MapBlockIdFromCoords{0, 0, 0, 0},
                                MapBlockIdFromCoords{46, 0, -7, -117440466}));

struct PosInsideTestData {
  Pos<int> a;    // input
  Pos<int> b;    // input
  Pos<int> x;    // input
  bool expected; // expected
};

class PosInsideTest : public ::testing::TestWithParam<PosInsideTestData> {};

TEST_P(PosInsideTest, Correct) {
  const auto &param = GetParam();
  EXPECT_THAT(param.x.inside(param.a, param.b), Eq(param.expected));
}

INSTANTIATE_TEST_SUITE_P(
    PosInsideTests, PosInsideTest,
    Values(
        // Point is inside range (at begin)
        PosInsideTestData{Pos<int>(0, 0, 0), Pos<int>(1, 1, 1),
                          Pos<int>(0, 0, 0), true},

        // Point is inside range.
        PosInsideTestData{Pos<int>(-1, -1, -1), Pos<int>(1, 1, 1),
                          Pos<int>(0, 0, 0), true},

        // "begin" of range is inclusive, "end" of range is exclusive
        PosInsideTestData{Pos<int>(0, 0, 0), Pos<int>(0, 0, 0),
                          Pos<int>(0, 0, 0), false},

        // a, b coords must be pre-sorted.
        PosInsideTestData{Pos<int>(1, 1, 1), Pos<int>(0, 0, 0),
                          Pos<int>(0, 0, 0), false},

        // "z" is outside range.
        PosInsideTestData{Pos<int>(0, 0, 0), Pos<int>(1, 1, 1),
                          Pos<int>(0, 0, -2), false},

        // "y" is outside range.
        PosInsideTestData{Pos<int>(0, 0, 0), Pos<int>(1, 1, 1),
                          Pos<int>(0, -3, 0), false},

        // "x" is outside range.
        PosInsideTestData{Pos<int>(0, 0, 0), Pos<int>(1, 1, 1),
                          Pos<int>(-1, 0, 0), false}

        ));

struct PosSortTestData {
  Pos<int> a_in;
  Pos<int> b_in;
  Pos<int> a_expected;
  Pos<int> b_expected;
};

class PosSortTest : public ::testing::TestWithParam<PosSortTestData> {};

TEST_P(PosSortTest, Correct) {
  const auto &param = GetParam();
  Pos<int> a = param.a_in;
  Pos<int> b = param.b_in;
  a.sort(&b);

  EXPECT_THAT(a, Eq(param.a_expected));
  EXPECT_THAT(b, Eq(param.b_expected));
}

INSTANTIATE_TEST_SUITE_P(
    PosSortTests, PosSortTest,
    Values(
        // Already sorted
        PosSortTestData{Pos<int>(1, 2, 3), Pos<int>(4, 5, 6), Pos<int>(1, 2, 3),
                        Pos<int>(4, 5, 6)},

        // Needs to swap only X
        PosSortTestData{Pos<int>(4, 2, 3), Pos<int>(1, 5, 6), Pos<int>(1, 2, 3),
                        Pos<int>(4, 5, 6)},

        // Needs to swap only Y
        PosSortTestData{Pos<int>(1, 5, 3), Pos<int>(4, 2, 6), Pos<int>(1, 2, 3),
                        Pos<int>(4, 5, 6)},

        // Needs to swap only Z
        PosSortTestData{Pos<int>(1, 2, 6), Pos<int>(4, 5, 3), Pos<int>(1, 2, 3),
                        Pos<int>(4, 5, 6)},

        // Need to swap all dimensions.
        PosSortTestData{Pos<int>(4, 5, 6), Pos<int>(1, 2, 3), Pos<int>(1, 2, 3),
                        Pos<int>(4, 5, 6)} //
        ));

struct PosFromStringTestData {
  std::string_view input;
  Pos<int> expected_value;
};

class PosFromStringTest
    : public ::testing::TestWithParam<PosFromStringTestData> {};

TEST_P(PosFromStringTest, ValidInput) {
  const auto &param = GetParam();
  Pos<int> p;
  EXPECT_THAT(p.parse(param.input), IsTrue());
  EXPECT_THAT(p, Eq(param.expected_value));
}

INSTANTIATE_TEST_SUITE_P(
    PosFromStringTests, PosFromStringTest,
    Values(PosFromStringTestData{"1,2,3", Pos<int>(1, 2, 3)},
           PosFromStringTestData{" 1,2,3", Pos<int>(1, 2, 3)},
           PosFromStringTestData{"1,2,3,", Pos<int>(1, 2, 3)},
           PosFromStringTestData{"1,2,3 abc", Pos<int>(1, 2, 3)},
           PosFromStringTestData{"-1,-2,-3", Pos<int>(-1, -2, -3)},
           PosFromStringTestData{"-32768,32767,99",
                                 Pos<int>(-32768, 32767, 99)}));

struct PosFromInvalidStringTestData {
  std::string_view input;
};

class PosFromInvalidStringTest
    : public ::testing::TestWithParam<PosFromInvalidStringTestData> {};

TEST_P(PosFromInvalidStringTest, InvalidInput) {
  const auto &param = GetParam();
  Pos<int> p(99, 98, 97);
  EXPECT_THAT(p.parse(param.input), IsFalse());
  EXPECT_THAT(p, Eq(Pos<int>(99, 98, 97)));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    PosFromInvalidStringTests, PosFromInvalidStringTest,
    Values(PosFromInvalidStringTestData{"1,x,3"},
           PosFromInvalidStringTestData{"1"},
           PosFromInvalidStringTestData{"1,2"},
           PosFromInvalidStringTestData{"1,,3"},
           PosFromInvalidStringTestData{",,"},
           PosFromInvalidStringTestData{" 1 , 2 , 3"}
    ));
// clang-format on

struct RoundTripBlockIdTestData {
  int64_t mapblock_id;
};

class RoundTripBlockIdTest
    : public ::testing::TestWithParam<RoundTripBlockIdTestData> {};

TEST_P(RoundTripBlockIdTest, Correct) {
  const auto &param = GetParam();
  EXPECT_THAT(MapBlockPos(param.mapblock_id).MapBlockId(),
              Eq(param.mapblock_id));
}

INSTANTIATE_TEST_SUITE_P(RoundTripBlockIdTests, RoundTripBlockIdTest,
                         Values(0, 1, -1, 16, -16, 17, -17, 257, -315, 1022,
                                1023, 1024, 1025, -1022, -1023, -1024, -1025,
                                4095, -4095, -4096, -117440466,

                                // Random samples from production.
                                5754503030, 16609370179, -688259021, -805859109,
                                16156435412, 31406970694, 1107268108,
                                7597330904, -957070954, 25887007828,

                                -32497465456, // min observed in production.
                                32497498001   // max observed in production.

                                ));
