#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/map_reader/minegeld.h"

using ::testing::Eq;
using ::testing::Values;

struct MinegeldItem {
  const char *input;
  uint64_t expected;
};

class MinegeldTest : public ::testing::TestWithParam<MinegeldItem> {};

TEST_P(MinegeldTest, Correct) {
  const auto &param = GetParam();
  EXPECT_THAT(ParseCurrencyMinegeld(param.input), Eq(param.expected));
}

INSTANTIATE_TEST_SUITE_P(MinegeldTests, MinegeldTest,
                         Values(MinegeldItem{"", 0}, MinegeldItem{"air", 0},
                                MinegeldItem{"currency:minegeld_10", 10},
                                MinegeldItem{"currency:minegeld_10 5", 50},
                                MinegeldItem{"currency:minegeld_25 3", 75},
                                MinegeldItem{"currency:minegeld_100 4", 400}));
