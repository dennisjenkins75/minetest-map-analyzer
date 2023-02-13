#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/3dmatrix/3dmatrix.h"

using ::testing::Eq;
using ::testing::IsEmpty;

TEST(Sparse3DMatrix, General) {
  Sparse3DMatrix<std::string> foo;

  EXPECT_THAT(foo.Ref(751, -325, -1987), IsEmpty());
  foo.Ref(751, -325, -1987) = "hello";
  EXPECT_THAT(foo.Ref(751, -325, -1987), Eq("hello"));
}
