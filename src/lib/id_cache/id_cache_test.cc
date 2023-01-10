#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/id_cache/id_cache.h"

using ::testing::Eq;

TEST(IdCache, General) {
  IdCache foo;

  EXPECT_THAT(foo.Add("air"), Eq(0));
  EXPECT_THAT(foo.Add("foo"), Eq(1));
  EXPECT_THAT(foo.Add("bar"), Eq(2));
  EXPECT_THAT(foo.Add("baz"), Eq(3));

  EXPECT_THAT(foo.Get(0), Eq("air"));
  EXPECT_THAT(foo.Get(1), Eq("foo"));
  EXPECT_THAT(foo.Get(2), Eq("bar"));
  EXPECT_THAT(foo.Get(3), Eq("baz"));
}
