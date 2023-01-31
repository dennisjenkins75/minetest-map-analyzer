#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/id_map/id_map.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Pair;

TEST(IdMap, General) {
  IdMap<bool> foo;

  EXPECT_THAT(foo.Add("air"), Eq(0));
  EXPECT_THAT(foo.Add("foo"), Eq(1));
  EXPECT_THAT(foo.Add("bar"), Eq(2));
  EXPECT_THAT(foo.Add("baz"), Eq(3));

  // Re-add existing item, get cached value.
  EXPECT_THAT(foo.Add("foo"), Eq(1));

  EXPECT_THAT(foo.Get(0).key, Eq("air"));
  EXPECT_THAT(foo.Get(1).key, Eq("foo"));
  EXPECT_THAT(foo.Get(2).key, Eq("bar"));
  EXPECT_THAT(foo.Get(3).key, Eq("baz"));
}

TEST(IdMap, Dirty) {
  IdMap<bool> foo;

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

TEST(ThreadLocalIdMap, General) {
  IdMap<int> top;
  ThreadLocalIdMap<int> a(top);
  ThreadLocalIdMap<int> b(top);

  EXPECT_THAT(a.Add("air"), Eq(0));
  EXPECT_THAT(b.Add("air"), Eq(0));

  EXPECT_THAT(a.Add("foo"), Eq(1));
  EXPECT_THAT(b.Add("bar"), Eq(2));

  EXPECT_THAT(a.Add("bar"), Eq(2));
  EXPECT_THAT(b.Add("foo"), Eq(1));
}

TEST(IdMap, ExtraInfoCallback) {
  struct Extra {
    Extra() : a(-2), b(-2) {}
    Extra(int a_, int b_) : a(a_), b(b_) {}
    int a;
    int b;

    bool operator==(const Extra &other) const {
      return (a == other.a) && (b == other.b);
    }
  };

  const auto callback = [](const std::string &key) -> Extra {
    if (key == "air") {
      return Extra(0, 0);
    } else if (key == "cat") {
      return Extra(1, 3);
    } else if (key == "dog") {
      return Extra(4, 9);
    } else {
      return Extra(-1, -1);
    }
  };

  IdMap<Extra> foo(callback);

  EXPECT_THAT(foo.Add("air"), Eq(0));
  EXPECT_THAT(foo.Add("fox"), Eq(1));
  EXPECT_THAT(foo.Add("dog"), Eq(2));

  EXPECT_THAT(foo.Get("fox").extra, Eq(Extra(-1, -1)));
  EXPECT_THAT(foo.Get("dog").extra, Eq(Extra(4, 9)));

  // "cat" was NOT inserted yet, so its Extra info should not set yet.
  // Note the default constructor for `Extra`.
  EXPECT_THAT(foo.Get("cat").extra, Eq(Extra()));
}
