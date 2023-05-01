#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/name_filter/name_filter.h"

using ::testing::IsFalse;
using ::testing::IsTrue;

TEST(NameFilter, General) {
  NameFilter nf;
  nf.Add("technic:");
  nf.Add("home_decor:.*");
  nf.Add("default:ladder");

  EXPECT_THAT(nf.Search("default:stone"), IsFalse());
  EXPECT_THAT(nf.Search("default:ladder"), IsTrue());
  EXPECT_THAT(nf.Search("technic:foo"), IsFalse());
  EXPECT_THAT(nf.Search("not_technic:hv_quarry"), IsFalse());
  EXPECT_THAT(nf.Search("home_decor:anything"), IsTrue());
}

TEST(NameFilter, Exceptions) {
  NameFilter nf;
  nf.Add("technic:.*");
  nf.Add("!technic:mineral.*");

  EXPECT_THAT(nf.Search("default:stone"), IsFalse());
  EXPECT_THAT(nf.Search("technic:foo"), IsTrue());
  EXPECT_THAT(nf.Search("technic:mineral_lead"), IsFalse());
}

TEST(NameFilter, FromStream) {
  std::stringstream ss;
  ss << R"(
# This is a comment

technic:
home_decor:.*
default:ladder
)";
  ss.seekg(0, ss.beg);

  NameFilter nf;
  nf.Load(ss);

  EXPECT_THAT(nf.Search("default:stone"), IsFalse());
  EXPECT_THAT(nf.Search("default:ladder"), IsTrue());
  EXPECT_THAT(nf.Search("technic:foo"), IsFalse());
  EXPECT_THAT(nf.Search("not_technic:hv_quarry"), IsFalse());
  EXPECT_THAT(nf.Search("home_decor:anything"), IsTrue());
  EXPECT_THAT(nf.Search("no_such_mod:lol"), IsFalse());
  EXPECT_THAT(nf.Search("# This is a comment"), IsFalse());
  EXPECT_THAT(nf.Search(""), IsFalse());
}
