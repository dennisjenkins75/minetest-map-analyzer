#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/map_reader/blob_reader.h"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Ne;
using ::testing::Pointwise;
using ::testing::SizeIs;
using ::testing::StrEq;

TEST(BlobReader, General) {
  const std::vector<uint8_t> input = {
      0x3a, 0x2b,             // u16 (0x3a2b)
      0xff,                   // u8 (0xff)
      0x7f,                   // u8 (0x7f)

      0xde, 0xad, 0xbe, 0xef, // u32 (0xdeadbeef)
      0x05,                   // u8 (string len = 5)
      'h',  'e',  'l',  'l',  // "hello"
      'o',
  };

  BlobReader r(input);
  EXPECT_THAT(r.eof(), IsFalse());
  EXPECT_THAT(r, SizeIs(14));
  EXPECT_THAT(r.remaining(), Eq(14));

  // `void size_check()` is silent on success, throws on error.
  r.size_check(13, "foo"); // no exception thrown.
  r.size_check(14, "foo"); // no exception thrown.
  EXPECT_THROW(r.size_check(15, "foo"), SerializationError);

  // Begin reading, which will mutate state.
  EXPECT_THAT(r.read_u16("u16"), Eq(0x3a2b));
  EXPECT_THAT(r.remaining(), Eq(12));

  EXPECT_THAT(r.read_u8("u8"), Eq(255));
  EXPECT_THAT(r.remaining(), Eq(11));

  EXPECT_THAT(r.read_u8("u8"), Eq(127));
  EXPECT_THAT(r.remaining(), Eq(10));

  EXPECT_THAT(r.read_u32("u32"), Eq(0xdeadbeef));
  EXPECT_THAT(r.remaining(), Eq(6));

  uint8_t len = r.read_u8("strlen");
  EXPECT_THAT(len, Eq(5));
  EXPECT_THAT(r.remaining(), Eq(5));

  std::string s = r.read_str(len, "str");
  EXPECT_THAT(s, Eq("hello"));
  EXPECT_THAT(r.remaining(), Eq(0));

  // We should have reached EOF.
  EXPECT_THAT(r.eof(), IsTrue());

  EXPECT_THROW(r.read_u8("past_eof"), SerializationError);
}

// TODO: Need unit test for `BlobReader::decompress_zlib()`
// TODO: Need unit test for `BlobReader::read_line()`
