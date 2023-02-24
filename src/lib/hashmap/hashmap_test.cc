#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/hashmap/hashmap.h"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(HashMap, InitialIsEmpty) {
  HashMap<int, std::string> foo;
  EXPECT_THAT(foo, SizeIs(0));
}

TEST(HashMap, CreateOnAccess) {
  HashMap<int, std::string> foo;
  ASSERT_THAT(foo, SizeIs(0));

  foo.Ref(1);
  EXPECT_THAT(foo, SizeIs(1));
}

TEST(HashMap, AutoCreateNodeIsEmpty) {
  HashMap<int, std::string> foo;

  foo.Ref(1);
  EXPECT_THAT(foo.Ref(1), IsEmpty());
}

TEST(HashMap, CanSetValue) {
  HashMap<int, std::string> foo;

  foo.Ref(1) = "hello";
  EXPECT_THAT(foo.Ref(1), Eq("hello"));
}

TEST(HashMap, CanChangeValue) {
  HashMap<int, std::string> foo;

  foo.Ref(1) = "hello";
  EXPECT_THAT(foo.Ref(1), Eq("hello"));

  foo.Ref(1) = "bye";
  EXPECT_THAT(foo.Ref(1), Eq("bye"));
}

TEST(HashMap, SizeReporting) {
  HashMap<int, std::string> foo;

  for (int i = 0; i < 256; i++) {
    foo.Ref(i);
  }
  EXPECT_THAT(foo, SizeIs(256));
}

TEST(HashMap, ThreadConcurrency) {
  static constexpr size_t kThreads = 8;
  static constexpr size_t kPerThreadItems = 1024 * 1024;
  // 8M items should take ~6G of RAM.

  HashMap<int, int> matrix;
  std::vector<std::thread> threads;

  auto worker = [&matrix](size_t idx) -> void {
    for (size_t i = 0; i < kPerThreadItems; i++) {
      const size_t x = idx | (i << 8);
      matrix.Ref(x) = i;
    }
  };

  for (size_t i = 0; i < kThreads; ++i) {
    threads.push_back(std::thread(worker, i));
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_THAT(matrix, SizeIs(kThreads * kPerThreadItems));

  // use ASSERT, b/c we want the test to fail after the FIRST error.
  for (size_t y = 0; y < kThreads; ++y) {
    for (size_t x = 0; x < kPerThreadItems; ++x) {
      const size_t t = y | (x << 8);
      ASSERT_THAT(matrix.Ref(t), Eq(x));
    }
  }
}
