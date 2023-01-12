#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/id_map/id_map.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Pair;

TEST(IdMap, General) {
  IdMap foo;

  EXPECT_THAT(foo.Add("air"), Eq(0));
  EXPECT_THAT(foo.Add("foo"), Eq(1));
  EXPECT_THAT(foo.Add("bar"), Eq(2));
  EXPECT_THAT(foo.Add("baz"), Eq(3));

  // Re-add existing item, get cached value.
  EXPECT_THAT(foo.Add("foo"), Eq(1));

  EXPECT_THAT(foo.Get(0), Eq("air"));
  EXPECT_THAT(foo.Get(1), Eq("foo"));
  EXPECT_THAT(foo.Get(2), Eq("bar"));
  EXPECT_THAT(foo.Get(3), Eq("baz"));
}

TEST(IdMap, Dirty) {
  IdMap foo;

  EXPECT_THAT(foo.Add("air"), Eq(0));
  EXPECT_THAT(foo.Add("foo"), Eq(1));

  const auto d1 = foo.GetDirty();
  EXPECT_THAT(d1, ElementsAre(Pair(0, "air"), Pair(1, "foo")));

  const auto d2 = foo.GetDirty();
  EXPECT_THAT(d2, IsEmpty());

  EXPECT_THAT(foo.Add("bar"), Eq(2));
  EXPECT_THAT(foo.Add("baz"), Eq(3));

  const auto d3 = foo.GetDirty();
  EXPECT_THAT(d3, ElementsAre(Pair(2, "bar"), Pair(3, "baz")));
}
