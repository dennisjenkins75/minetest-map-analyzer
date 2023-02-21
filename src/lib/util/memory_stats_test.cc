#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/util/memory_stats.h"

using ::testing::Gt;

TEST(GetMemoryStats, Works) {
  const MemoryStats ms = GetMemoryStats();

  EXPECT_THAT(ms.rss, Gt(0));
  EXPECT_THAT(ms.vsize, Gt(0));
}
