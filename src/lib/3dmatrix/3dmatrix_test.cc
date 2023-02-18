#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/3dmatrix/3dmatrix.h"

using ::testing::Eq;
using ::testing::IsEmpty;

TEST(Sparse3DMatrix, General) {
  Sparse3DMatrix<std::string> foo;

  const MapBlockPos p1(751, -325, -1987);

  EXPECT_THAT(foo.Ref(p1), IsEmpty());
  foo.Ref(p1) = "hello";
  EXPECT_THAT(foo.Ref(p1), Eq("hello"));
}
