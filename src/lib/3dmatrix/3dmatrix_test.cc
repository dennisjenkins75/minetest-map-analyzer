#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/3dmatrix/3dmatrix.h"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(Sparse3DMatrix, InitialIsEmpty) {
  Sparse3DMatrix<int, std::string> foo;
  EXPECT_THAT(foo, SizeIs(0));
}

TEST(Sparse3DMatrix, CreateOnAccess) {
  Sparse3DMatrix<int, std::string> foo;
  ASSERT_THAT(foo, SizeIs(0));

  foo.Ref(1);
  EXPECT_THAT(foo, SizeIs(1));
}

TEST(Sparse3DMatrix, AutoCreateNodeIsEmpty) {
  Sparse3DMatrix<int, std::string> foo;

  foo.Ref(1);
  EXPECT_THAT(foo.Ref(1), IsEmpty());
}

TEST(Sparse3DMatrix, CanSetValue) {
  Sparse3DMatrix<int, std::string> foo;

  foo.Ref(1) = "hello";
  EXPECT_THAT(foo.Ref(1), Eq("hello"));
}

TEST(Sparse3DMatrix, CanChangeValue) {
  Sparse3DMatrix<int, std::string> foo;

  foo.Ref(1) = "hello";
  EXPECT_THAT(foo.Ref(1), Eq("hello"));

  foo.Ref(1) = "bye";
  EXPECT_THAT(foo.Ref(1), Eq("bye"));
}

TEST(Sparse3DMatrix, SizeReporting) {
  Sparse3DMatrix<int, std::string> foo;

  for (int i = 0; i < 256; i++) {
    foo.Ref(i);
  }
  EXPECT_THAT(foo, SizeIs(256));
}

TEST(Sparse3DMatrix, ThreadConcurrency) {
  static constexpr size_t kThreads = 8;
  static constexpr size_t kPerThreadItems = 1024 * 1024;
  // 8M items should take ~6G of RAM.

  Sparse3DMatrix<int, int> matrix;
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
